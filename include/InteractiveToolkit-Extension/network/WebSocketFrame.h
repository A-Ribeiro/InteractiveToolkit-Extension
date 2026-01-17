#pragma once

#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/EventCore/Callback.h>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

namespace ITKExtension
{
    namespace Network
    {
        // WebSocket frame opcodes (RFC 6455)
        enum class WebSocketOpcode : uint8_t
        {
            Continuation = 0x00,
            Text = 0x01,
            Binary = 0x02,
            // 0x03-0x07 reserved for further non-control frames
            Close = 0x08,
            Ping = 0x09,
            Pong = 0x0A
            // 0x0B-0x0F reserved for further control frames
        };

        // WebSocket close codes (RFC 6455)
        enum class WebSocketCloseCode : uint16_t
        {
            Normal = 1000,
            GoingAway = 1001,
            ProtocolError = 1002,
            UnsupportedData = 1003,
            NoStatusReceived = 1005,
            AbnormalClosure = 1006,
            InvalidPayloadData = 1007,
            PolicyViolation = 1008,
            MessageTooBig = 1009,
            MandatoryExtension = 1010,
            InternalError = 1011,
            ServiceRestart = 1012,
            TryAgainLater = 1013,
            BadGateway = 1502,
            TLSHandshake = 1015
        };

        // WebSocket frame parser state
        enum class WebSocketFrameParserState : int
        {
            ReadingHeader = 0,
            ReadingExtendedLength16,
            ReadingExtendedLength64,
            ReadingMaskKey,
            ReadingPayload,
            Complete,
            Error
        };

        // WebSocket frame writer state
        enum class WebSocketFrameWriterState : int
        {
            None = 0,
            WritingHeader,
            WritingPayload,
            Complete,
            Error
        };

        // Forward declaration
        class WebSocketFrame;

        // WebSocket Frame Parser - parses incoming WebSocket frames
        class WebSocketFrameParser
        {
        private:
            // Header bytes
            uint8_t header_buffer[14]; // Max header size: 2 + 8 (extended length) + 4 (mask)
            uint32_t header_buffer_pos;
            uint32_t header_expected_size;

            // Parsed header fields
            bool fin;
            bool rsv1, rsv2, rsv3;
            WebSocketOpcode opcode;
            bool masked;
            uint64_t payload_length;
            uint8_t mask_key[4];

            // Payload
            std::vector<uint8_t> payload_buffer;
            uint64_t payload_bytes_read;

            // Maximum allowed payload size
            uint64_t max_payload_size;

            WebSocketFrameParserState parseHeader();
            void unmaskPayload();

        public:
            WebSocketFrameParserState state;

            // Delete copy constructor and assignment operator
            WebSocketFrameParser(const WebSocketFrameParser &) = delete;
            WebSocketFrameParser &operator=(const WebSocketFrameParser &) = delete;

            WebSocketFrameParser(uint64_t max_payload_size = 16 * 1024 * 1024); // 16MB default
            ~WebSocketFrameParser() = default;

            void reset();

            // Insert data and parse, returns number of bytes consumed
            WebSocketFrameParserState insertData(const uint8_t *data, uint32_t size, uint32_t *inserted_bytes);

            // Get parsed frame data
            bool isFin() const { return fin; }
            WebSocketOpcode getOpcode() const { return opcode; }
            bool isMasked() const { return masked; }
            uint64_t getPayloadLength() const { return payload_length; }
            const std::vector<uint8_t> &getPayload() const { return payload_buffer; }
            std::vector<uint8_t> &getPayloadMutable() { return payload_buffer; }
        };

        // WebSocket Frame Writer - writes outgoing WebSocket frames
        class WebSocketFrameWriter
        {
        private:
            // Header
            uint8_t header_buffer[14];
            uint32_t header_size;
            uint32_t header_written;

            // Payload reference
            const uint8_t *payload_data;
            uint64_t payload_size;
            uint64_t payload_written;

            // Mask key (for client frames)
            bool use_mask;
            uint8_t mask_key[4];

            // Masked payload buffer (only used when masking is required)
            std::vector<uint8_t> masked_payload;

            void buildHeader(bool fin, WebSocketOpcode opcode, uint64_t payload_length, bool mask);

        public:
            WebSocketFrameWriterState state;

            // Delete copy constructor and assignment operator
            WebSocketFrameWriter(const WebSocketFrameWriter &) = delete;
            WebSocketFrameWriter &operator=(const WebSocketFrameWriter &) = delete;

            WebSocketFrameWriter();
            ~WebSocketFrameWriter() = default;

            void reset();

            // Initialize frame for writing
            // mask = true for client frames (RFC 6455 requires client-to-server frames to be masked)
            bool startFrame(bool fin, WebSocketOpcode opcode, const uint8_t *payload, uint64_t payload_length, bool mask);

            // Get current data to write
            const uint8_t *getData() const;
            uint32_t getDataSize() const;

            // Advance to next chunk
            bool next();
        };

        // WebSocket Frame - represents a complete WebSocket frame
        class WebSocketFrame
        {
        public:
            bool fin;
            WebSocketOpcode opcode;
            std::vector<uint8_t> payload;

            WebSocketFrame();
            ~WebSocketFrame() = default;

            // Delete copy constructor and assignment operator
            WebSocketFrame(const WebSocketFrame &) = delete;
            WebSocketFrame &operator=(const WebSocketFrame &) = delete;

            // Move constructor and assignment
            WebSocketFrame(WebSocketFrame &&other) noexcept;
            WebSocketFrame &operator=(WebSocketFrame &&other) noexcept;

            void clear();

            // Factory methods for creating frames
            static std::shared_ptr<WebSocketFrame> CreateText(const std::string &text, bool fin = true);
            static std::shared_ptr<WebSocketFrame> CreateBinary(const std::vector<uint8_t> &data, bool fin = true);
            static std::shared_ptr<WebSocketFrame> CreateBinary(const uint8_t *data, size_t size, bool fin = true);
            static std::shared_ptr<WebSocketFrame> CreatePing(const std::string &data = "");
            static std::shared_ptr<WebSocketFrame> CreatePing(const uint8_t *data, size_t size);
            static std::shared_ptr<WebSocketFrame> CreatePong(const std::string &data = "");
            static std::shared_ptr<WebSocketFrame> CreatePong(const uint8_t *data, size_t size);
            static std::shared_ptr<WebSocketFrame> CreateClose(WebSocketCloseCode code = WebSocketCloseCode::Normal,
                                                                const std::string &reason = "");

            // Content extraction methods
            bool isText() const { return opcode == WebSocketOpcode::Text; }
            bool isBinary() const { return opcode == WebSocketOpcode::Binary; }
            bool isPing() const { return opcode == WebSocketOpcode::Ping; }
            bool isPong() const { return opcode == WebSocketOpcode::Pong; }
            bool isClose() const { return opcode == WebSocketOpcode::Close; }
            bool isContinuation() const { return opcode == WebSocketOpcode::Continuation; }
            bool isControlFrame() const { return (static_cast<uint8_t>(opcode) & 0x08) != 0; }
            bool isDataFrame() const { return !isControlFrame(); }

            // Get content as text (for text frames)
            bool getText(std::string *out) const;

            // Get content as binary
            bool getBinary(std::vector<uint8_t> *out) const;

            // Get close code and reason (for close frames)
            bool getCloseInfo(WebSocketCloseCode *code, std::string *reason) const;

            // Get ping/pong data
            bool getPingData(std::vector<uint8_t> *out) const;
            bool getPongData(std::vector<uint8_t> *out) const;
        };

    }
}
