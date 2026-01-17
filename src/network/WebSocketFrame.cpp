#include <InteractiveToolkit-Extension/network/WebSocketFrame.h>
#include <cstring>
#include <algorithm>
#include <InteractiveToolkit/ITKCommon/Random.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {
        //
        // WebSocketFrameParser Implementation
        //

        WebSocketFrameParser::WebSocketFrameParser(uint64_t max_payload_size)
        {
            this->max_payload_size = max_payload_size;
            reset();
        }

        void WebSocketFrameParser::reset()
        {
            state = WebSocketFrameParserState::ReadingHeader;
            header_buffer_pos = 0;
            header_expected_size = 2; // Minimum header size
            fin = false;
            rsv1 = rsv2 = rsv3 = false;
            opcode = WebSocketOpcode::Continuation;
            masked = false;
            payload_length = 0;
            memset(mask_key, 0, 4);
            payload_buffer.clear();
            payload_bytes_read = 0;
        }

        WebSocketFrameParserState WebSocketFrameParser::parseHeader()
        {
            if (header_buffer_pos < 2)
                return state;

            // Parse first byte
            uint8_t byte0 = header_buffer[0];
            fin = (byte0 & 0x80) != 0;
            rsv1 = (byte0 & 0x40) != 0;
            rsv2 = (byte0 & 0x20) != 0;
            rsv3 = (byte0 & 0x10) != 0;
            opcode = static_cast<WebSocketOpcode>(byte0 & 0x0F);

            // RSV bits must be 0 unless an extension defines their meaning
            if (rsv1 || rsv2 || rsv3)
            {
                state = WebSocketFrameParserState::Error;
                return state;
            }

            // Parse second byte
            uint8_t byte1 = header_buffer[1];
            masked = (byte1 & 0x80) != 0;
            uint8_t payload_len_7bit = byte1 & 0x7F;

            // Calculate expected header size
            uint32_t expected = 2;
            if (payload_len_7bit == 126)
                expected += 2;
            else if (payload_len_7bit == 127)
                expected += 8;
            if (masked)
                expected += 4;

            header_expected_size = expected;

            // Need more header data?
            if (header_buffer_pos < header_expected_size)
            {
                if (payload_len_7bit == 126)
                    state = WebSocketFrameParserState::ReadingExtendedLength16;
                else if (payload_len_7bit == 127)
                    state = WebSocketFrameParserState::ReadingExtendedLength64;
                else if (masked)
                    state = WebSocketFrameParserState::ReadingMaskKey;
                return state;
            }

            // Parse extended length
            uint32_t offset = 2;
            if (payload_len_7bit == 126)
            {
                payload_length = ((uint16_t)header_buffer[offset] << 8) |
                                 (uint16_t)header_buffer[offset + 1];
                offset += 2;
            }
            else if (payload_len_7bit == 127)
            {
                payload_length = 0;
                for (int i = 0; i < 8; i++)
                    payload_length = (payload_length << 8) | header_buffer[offset + i];
                offset += 8;
                // Most significant bit must be 0
                if (payload_length & 0x8000000000000000ULL)
                {
                    state = WebSocketFrameParserState::Error;
                    return state;
                }
            }
            else
                payload_length = payload_len_7bit;

            // Check payload size limit
            if (payload_length > max_payload_size)
            {
                state = WebSocketFrameParserState::Error;
                return state;
            }

            // Parse mask key if present
            if (masked)
            {
                memcpy(mask_key, header_buffer + offset, 4);
                offset += 4;
            }

            // Prepare for payload
            payload_buffer.clear();
            if (payload_length > 0)
                payload_buffer.reserve((size_t)payload_length);
            payload_bytes_read = 0;

            if (payload_length == 0)
                state = WebSocketFrameParserState::Complete;
            else
                state = WebSocketFrameParserState::ReadingPayload;

            return state;
        }

        void WebSocketFrameParser::unmaskPayload()
        {
            if (!masked || payload_buffer.empty())
                return;

            for (size_t i = 0; i < payload_buffer.size(); i++)
                payload_buffer[i] ^= mask_key[i % 4];
        }

        WebSocketFrameParserState WebSocketFrameParser::insertData(const uint8_t *data, uint32_t size, uint32_t *inserted_bytes)
        {
            *inserted_bytes = 0;

            if (state == WebSocketFrameParserState::Error ||
                state == WebSocketFrameParserState::Complete)
                return state;

            uint32_t pos = 0;

            // Read header bytes
            while (pos < size && header_buffer_pos < header_expected_size)
            {
                header_buffer[header_buffer_pos++] = data[pos++];
                (*inserted_bytes)++;

                if (header_buffer_pos >= 2 && header_buffer_pos < header_expected_size)
                {
                    parseHeader();
                    if (state == WebSocketFrameParserState::Error)
                        return state;
                }
                else if (header_buffer_pos >= header_expected_size)
                {
                    parseHeader();
                    if (state == WebSocketFrameParserState::Error)
                        return state;
                    break;
                }
            }

            // Read payload
            if (state == WebSocketFrameParserState::ReadingPayload && pos < size)
            {
                uint64_t remaining = payload_length - payload_bytes_read;
                uint64_t to_read = (std::min)((uint64_t)(size - pos), remaining);

                payload_buffer.insert(payload_buffer.end(), data + pos, data + pos + to_read);
                payload_bytes_read += to_read;
                pos += (uint32_t)to_read;
                *inserted_bytes = pos;

                if (payload_bytes_read >= payload_length)
                {
                    unmaskPayload();
                    state = WebSocketFrameParserState::Complete;
                }
            }

            return state;
        }

        //
        // WebSocketFrameWriter Implementation
        //

        WebSocketFrameWriter::WebSocketFrameWriter()
        {
            reset();
        }

        void WebSocketFrameWriter::reset()
        {
            state = WebSocketFrameWriterState::None;
            header_size = 0;
            header_written = 0;
            payload_data = nullptr;
            payload_size = 0;
            payload_written = 0;
            use_mask = false;
            memset(mask_key, 0, 4);
            masked_payload.clear();
        }

        void WebSocketFrameWriter::buildHeader(bool fin, WebSocketOpcode opcode, uint64_t payload_length, bool mask)
        {
            uint32_t pos = 0;

            // First byte: FIN + opcode
            header_buffer[pos++] = (fin ? 0x80 : 0x00) | static_cast<uint8_t>(opcode);

            // Second byte: MASK + payload length
            uint8_t mask_bit = mask ? 0x80 : 0x00;
            if (payload_length <= 125)
                header_buffer[pos++] = mask_bit | (uint8_t)payload_length;
            else if (payload_length <= 65535)
            {
                header_buffer[pos++] = mask_bit | 126;
                header_buffer[pos++] = (uint8_t)((payload_length >> 8) & 0xFF);
                header_buffer[pos++] = (uint8_t)(payload_length & 0xFF);
            }
            else
            {
                header_buffer[pos++] = mask_bit | 127;
                for (int i = 7; i >= 0; i--)
                    header_buffer[pos++] = (uint8_t)((payload_length >> (i * 8)) & 0xFF);
            }

            // Add mask key if masking
            if (mask)
            {
                // Generate random mask key
                auto &rnd = *ITKCommon::Random32::Instance();
                for (int i = 0; i < 4; i++)
                {
                    mask_key[i] = rnd.getRange<uint8_t>(0, 255);
                    header_buffer[pos++] = mask_key[i];
                }
            }

            header_size = pos;
        }

        bool WebSocketFrameWriter::startFrame(bool fin, WebSocketOpcode opcode, const uint8_t *payload, uint64_t payload_length, bool mask)
        {
            reset();

            buildHeader(fin, opcode, payload_length, mask);

            use_mask = mask;
            payload_size = payload_length;

            if (mask && payload_length > 0 && payload != nullptr)
            {
                // Create masked copy of payload
                masked_payload.resize((size_t)payload_length);
                for (size_t i = 0; i < payload_length; i++)
                    masked_payload[i] = payload[i] ^ mask_key[i % 4];
                payload_data = masked_payload.data();
            }
            else
                payload_data = payload;

            header_written = 0;
            payload_written = 0;
            state = WebSocketFrameWriterState::WritingHeader;

            return true;
        }

        const uint8_t *WebSocketFrameWriter::getData() const
        {
            if (state == WebSocketFrameWriterState::WritingHeader)
                return header_buffer + header_written;
            else if (state == WebSocketFrameWriterState::WritingPayload)
                return payload_data + payload_written;
            return nullptr;
        }

        uint32_t WebSocketFrameWriter::getDataSize() const
        {
            if (state == WebSocketFrameWriterState::WritingHeader)
                return header_size - header_written;
            else if (state == WebSocketFrameWriterState::WritingPayload)
            {
                uint64_t remaining = payload_size - payload_written;
                // Cap at 32-bit for socket writes
                return (uint32_t)(std::min)(remaining, (uint64_t)0xFFFFFFFF);
            }
            return 0;
        }

        bool WebSocketFrameWriter::next()
        {
            if (state == WebSocketFrameWriterState::WritingHeader)
            {
                header_written = header_size;
                if (payload_size > 0)
                    state = WebSocketFrameWriterState::WritingPayload;
                else
                    state = WebSocketFrameWriterState::Complete;
                return true;
            }
            else if (state == WebSocketFrameWriterState::WritingPayload)
            {
                payload_written = payload_size;
                state = WebSocketFrameWriterState::Complete;
                return true;
            }
            return false;
        }

        //
        // WebSocketFrame Implementation
        //

        WebSocketFrame::WebSocketFrame()
        {
            clear();
        }

        WebSocketFrame::WebSocketFrame(WebSocketFrame &&other) noexcept
            : fin(other.fin), opcode(other.opcode), payload(std::move(other.payload))
        {
            other.clear();
        }

        WebSocketFrame &WebSocketFrame::operator=(WebSocketFrame &&other) noexcept
        {
            if (this != &other)
            {
                fin = other.fin;
                opcode = other.opcode;
                payload = std::move(other.payload);
                other.clear();
            }
            return *this;
        }

        void WebSocketFrame::clear()
        {
            fin = true;
            opcode = WebSocketOpcode::Text;
            payload.clear();
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreateText(const std::string &text, bool fin)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = fin;
            frame->opcode = WebSocketOpcode::Text;
            frame->payload.assign(text.begin(), text.end());
            return frame;
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreateBinary(const std::vector<uint8_t> &data, bool fin)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = fin;
            frame->opcode = WebSocketOpcode::Binary;
            frame->payload = data;
            return frame;
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreateBinary(const uint8_t *data, size_t size, bool fin)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = fin;
            frame->opcode = WebSocketOpcode::Binary;
            frame->payload.assign(data, data + size);
            return frame;
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreatePing(const std::string &data)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = true;
            frame->opcode = WebSocketOpcode::Ping;
            frame->payload.assign(data.begin(), data.end());
            return frame;
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreatePing(const uint8_t *data, size_t size)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = true;
            frame->opcode = WebSocketOpcode::Ping;
            if (data != nullptr && size > 0)
                frame->payload.assign(data, data + size);
            return frame;
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreatePong(const std::string &data)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = true;
            frame->opcode = WebSocketOpcode::Pong;
            frame->payload.assign(data.begin(), data.end());
            return frame;
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreatePong(const uint8_t *data, size_t size)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = true;
            frame->opcode = WebSocketOpcode::Pong;
            if (data != nullptr && size > 0)
                frame->payload.assign(data, data + size);
            return frame;
        }

        std::shared_ptr<WebSocketFrame> WebSocketFrame::CreateClose(WebSocketCloseCode code, const std::string &reason)
        {
            auto frame = std::make_shared<WebSocketFrame>();
            frame->fin = true;
            frame->opcode = WebSocketOpcode::Close;

            // Close frame payload: 2 bytes for code + optional reason string
            frame->payload.reserve(2 + reason.size());
            uint16_t code_val = static_cast<uint16_t>(code);
            frame->payload.push_back((uint8_t)((code_val >> 8) & 0xFF));
            frame->payload.push_back((uint8_t)(code_val & 0xFF));
            frame->payload.insert(frame->payload.end(), reason.begin(), reason.end());

            return frame;
        }

        bool WebSocketFrame::getText(std::string *out) const
        {
            if (!isText() && !isContinuation())
                return false;
            if (out)
                out->assign(payload.begin(), payload.end());
            return true;
        }

        bool WebSocketFrame::getBinary(std::vector<uint8_t> *out) const
        {
            if (!isBinary() && !isContinuation())
                return false;
            if (out)
                *out = payload;
            return true;
        }

        bool WebSocketFrame::getCloseInfo(WebSocketCloseCode *code, std::string *reason) const
        {
            if (!isClose())
                return false;

            if (payload.size() >= 2)
            {
                if (code)
                {
                    uint16_t code_val = ((uint16_t)payload[0] << 8) | (uint16_t)payload[1];
                    *code = static_cast<WebSocketCloseCode>(code_val);
                }
                if (reason && payload.size() > 2)
                    reason->assign(payload.begin() + 2, payload.end());
                else if (reason)
                    reason->clear();
            }
            else
            {
                if (code)
                    *code = WebSocketCloseCode::NoStatusReceived;
                if (reason)
                    reason->clear();
            }

            return true;
        }

        bool WebSocketFrame::getPingData(std::vector<uint8_t> *out) const
        {
            if (!isPing())
                return false;
            if (out)
                *out = payload;
            return true;
        }

        bool WebSocketFrame::getPongData(std::vector<uint8_t> *out) const
        {
            if (!isPong())
                return false;
            if (out)
                *out = payload;
            return true;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif
