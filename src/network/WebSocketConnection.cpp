#include <InteractiveToolkit-Extension/network/WebSocketConnection.h>
#include <InteractiveToolkit-Extension/encoding/Base64.h>
#include <InteractiveToolkit-Extension/hashing/SHA1.h>
#include <InteractiveToolkit/Platform/SocketTCP.h>
#include <InteractiveToolkit/ITKCommon/StringUtil.h>
#include <InteractiveToolkit/ITKCommon/Random.h>

#include <cstring>
#include <algorithm>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {
        // WebSocket magic GUID (RFC 6455)
        static const char *WEBSOCKET_MAGIC_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        WebSocketConnection::WebSocketConnection(std::shared_ptr<Platform::SocketTCP> socket,
                                                 WebSocketConnectionMode mode,
                                                 bool strategy_read_write_until_socket_would_block,
                                                 uint32_t socket_buffer_size,
                                                 uint32_t parser_buffer_size,
                                                 uint32_t parser_max_header_count,
                                                 uint32_t writer_buffer_size,
                                                 uint32_t writing_transfer_encoding_max_size,
                                                 uint64_t websocket_max_frame_size)
        {
            this->socket = socket;
            this->mode = mode;
            this->strategy_read_write_until_socket_would_block = strategy_read_write_until_socket_would_block;
            this->socket_buffer_size = socket_buffer_size;
            this->parser_buffer_size = parser_buffer_size;
            this->parser_max_header_count = parser_max_header_count;
            this->writer_buffer_size = writer_buffer_size;
            this->writing_transfer_encoding_max_size = writing_transfer_encoding_max_size;
            this->websocket_max_frame_size = websocket_max_frame_size;

            this->socket_read_buffer = STL_Tools::make_unique<uint8_t[]>(socket_buffer_size);
            this->socket_read_buffer_remaining = STL_Tools::make_unique<uint8_t[]>(socket_buffer_size);
            this->socket_buffer_remaining_size = 0;

            this->http_written_offset = 0;
            this->current_write_offset = 0;

            // Initialize WebSocket frame parser/writer
            frame_parser = STL_Tools::make_unique<WebSocketFrameParser>(websocket_max_frame_size);
            frame_writer = STL_Tools::make_unique<WebSocketFrameWriter>();

            // Initialize HTTP parser/writer for handshake
            http_parser = STL_Tools::make_unique<HTTPParser>(parser_buffer_size, parser_max_header_count);
            http_writer = STL_Tools::make_unique<HTTPWriter>(writer_buffer_size);

            // Set initial state based on mode
            if (mode == WebSocketConnectionMode::Server)
            {
                handshake_request = HTTPRequestAsync::CreateShared();
                http_parser->initialize(false, handshake_request->getHTTPParserCallbacks());
                state = WebSocketConnectionState::Server_Handshaking;
            }
            else
            {
                handshake_response = HTTPResponseAsync::CreateShared();
                state = WebSocketConnectionState::Client_Handshaking;
            }

            reading_state = WebSocketReadingState::ReadingNone;
            writing_state = WebSocketWritingState::WritingNone;
        }

        WebSocketConnection::~WebSocketConnection()
        {
            close();
        }

        void WebSocketConnection::close()
        {
            socket->close();
            state = WebSocketConnectionState::Closed;
            reading_state = WebSocketReadingState::ReadingNone;
            writing_state = WebSocketWritingState::WritingNone;
        }

        void WebSocketConnection::closeWithCode(WebSocketCloseCode code, const std::string &reason)
        {
            if (state == WebSocketConnectionState::Connected ||
                state == WebSocketConnectionState::ProcessingConnection)
            {
                sendClose(code, reason);
            }
            close();
        }

        bool WebSocketConnection::clientGenerateWebSocketKey()
        {
            // Generate 16 random bytes and base64 encode them
            uint8_t random_bytes[16];
            // ITKCommon::Random32 rnd( ITKCommon::Random32::TypeDefinition::randomSeed() );
            auto &rnd = *ITKCommon::Random32::Instance();
            for (int i = 0; i < 16; i++)
                random_bytes[i] = rnd.getRange<uint8_t>(0, 255);
            return Encoding::Base64::EncodeToString(random_bytes, 16, &websocket_key);
        }

        bool WebSocketConnection::computeWebSocketAcceptKey(const std::string &key, std::string *accept_key)
        {
            // Concatenate key with magic GUID
            uint8_t sha1_digest[20];
            std::string concatenated = key + WEBSOCKET_MAGIC_GUID;
            ITKExtension::Hashing::SHA1::hash((const uint8_t *)concatenated.c_str(), concatenated.size(), sha1_digest);
            // Base64 encode the SHA-1 digest
            return Encoding::Base64::EncodeToString(sha1_digest, 20, accept_key);
        }

        bool WebSocketConnection::clientValidateWebSocketAcceptKey(const std::string &received_accept)
        {
            std::string expected_accept;
            if (!computeWebSocketAcceptKey(websocket_key, &expected_accept))
                return false;

            return received_accept == expected_accept;
        }

        bool WebSocketConnection::clientBeginHandshake(const std::string &host, const std::string &path,
                                                       const std::vector<std::string> &protocols,
                                                       const std::vector<std::pair<std::string, std::string>> &extra_headers)
        {
            if (mode != WebSocketConnectionMode::Client)
            {
                printf("[WebSocketConnection] Invalid operation: clientBeginHandshake called in server mode\n");
                state = WebSocketConnectionState::Error;
                return false;
            }

            if (state != WebSocketConnectionState::Client_Handshaking)
            {
                printf("[WebSocketConnection] Invalid operation: clientBeginHandshake called in wrong state\n");
                state = WebSocketConnectionState::Error;
                return false;
            }

            // Generate WebSocket key
            if (!clientGenerateWebSocketKey())
            {
                printf("[WebSocketConnection] Failed to generate WebSocket key\n");
                state = WebSocketConnectionState::Error;
                return false;
            }

            // Create handshake request
            handshake_request = HTTPRequestAsync::CreateShared();
            handshake_request->setDefault(host, "GET", path, "HTTP/1.1");
            handshake_request->setHeader("Upgrade", "websocket");
            handshake_request->setHeader("Connection", "Upgrade");
            handshake_request->setHeader("Sec-WebSocket-Key", websocket_key);
            handshake_request->setHeader("Sec-WebSocket-Version", "13");

            if (!protocols.empty())
            {
                std::string protocol_str;
                for (size_t i = 0; i < protocols.size(); i++)
                {
                    if (i > 0)
                        protocol_str += ", ";
                    protocol_str += protocols[i];
                }
                handshake_request->setHeader("Sec-WebSocket-Protocol", protocol_str);
            }

            for (const auto &header : extra_headers)
                handshake_request->setHeader(header.first, header.second);

            // Initialize HTTP writer
            http_writer->initialize(writing_transfer_encoding_max_size, handshake_request->getHTTPWriterCallbacks());
            http_writer->start_streaming();
            http_written_offset = 0;

            // Initialize parser for response
            http_parser->initialize(true, handshake_response->getHTTPParserCallbacks());

            return true;
        }

        bool WebSocketConnection::sendHandshakeResponse()
        {
            // Validate upgrade request
            std::string upgrade = handshake_request->getHeader("Upgrade");
            std::string connection = handshake_request->getHeader("Connection");
            std::string ws_key = handshake_request->getHeader("Sec-WebSocket-Key");
            std::string ws_version = handshake_request->getHeader("Sec-WebSocket-Version");

            // Case-insensitive comparison for headers
            std::transform(upgrade.begin(), upgrade.end(), upgrade.begin(), ::tolower);
            std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);

            if (upgrade != "websocket" || connection.find("upgrade") == std::string::npos)
            {
                printf("[WebSocketConnection] Invalid WebSocket upgrade request: Upgrade=%s, Connection=%s\n",
                       upgrade.c_str(), connection.c_str());
                state = WebSocketConnectionState::Error;
                return false;
            }

            if (ws_key.empty())
            {
                printf("[WebSocketConnection] Missing Sec-WebSocket-Key header\n");
                state = WebSocketConnectionState::Error;
                return false;
            }

            if (ws_version != "13")
            {
                printf("[WebSocketConnection] Unsupported WebSocket version: %s\n", ws_version.c_str());
                state = WebSocketConnectionState::Error;
                return false;
            }

            // Compute accept key
            std::string accept_key;
            if (!computeWebSocketAcceptKey(ws_key, &accept_key))
            {
                printf("[WebSocketConnection] Failed to compute accept key\n");
                state = WebSocketConnectionState::Error;
                return false;
            }

            // server receives the key and compute the accept key
            websocket_key = ws_key;
            websocket_accept_key = accept_key;

            // Create response
            handshake_response = HTTPResponseAsync::CreateShared();
            handshake_response->setDefault(101, "HTTP/1.1");
            handshake_response->setHeader("Upgrade", "websocket");
            handshake_response->setHeader("Connection", "Upgrade");
            handshake_response->setHeader("Sec-WebSocket-Accept", accept_key);

            // Copy protocol if requested
            std::string protocol = handshake_request->getHeader("Sec-WebSocket-Protocol");
            if (!protocol.empty())
            {
                // Just accept the first protocol for simplicity
                size_t comma = protocol.find(',');
                if (comma != std::string::npos)
                    protocol = protocol.substr(0, comma);
                // Trim whitespace
                while (!protocol.empty() && protocol.back() == ' ')
                    protocol.pop_back();
                handshake_response->setHeader("Sec-WebSocket-Protocol", protocol);
            }

            // Initialize HTTP writer
            http_writer->initialize(writing_transfer_encoding_max_size, handshake_response->getHTTPWriterCallbacks());
            http_writer->start_streaming();
            http_written_offset = 0;

            return true;
        }

        WebSocketProcessingResult WebSocketConnection::processHttpReading()
        {
            http_parser->checkOnlyStateTransition();
            if (http_parser->state == HTTPParserState::ReadingHeadersReady ||
                http_parser->state == HTTPParserState::Complete)
                return WebSocketProcessingResult::Completed;

            while (true)
            {
                uint32_t bytes_read = 0;
                Platform::SocketResult res;

                // Check if we have remaining data from previous read
                if (socket_buffer_remaining_size > 0)
                {
                    res = Platform::SOCKET_RESULT_OK;
                    std::swap(socket_read_buffer_remaining, socket_read_buffer);
                    bytes_read = socket_buffer_remaining_size;
                    socket_buffer_remaining_size = 0;
                }
                else
                    res = socket->read_buffer(socket_read_buffer.get(), socket_buffer_size, &bytes_read);

                if (res == Platform::SOCKET_RESULT_OK)
                {
                    uint32_t bytes_parsed;
                    HTTPParserState parser_state = http_parser->insertData(socket_read_buffer.get(), bytes_read, &bytes_parsed);

                    if (parser_state == HTTPParserState::Error)
                    {
                        printf("[WebSocketConnection] Error parsing HTTP handshake\n");
                        state = WebSocketConnectionState::Error;
                        return WebSocketProcessingResult::Error;
                    }

                    // Save remaining data
                    if (bytes_parsed < bytes_read)
                    {
                        socket_buffer_remaining_size = bytes_read - bytes_parsed;
                        memcpy(socket_read_buffer_remaining.get(),
                               socket_read_buffer.get() + bytes_parsed,
                               socket_buffer_remaining_size);
                    }

                    if (parser_state == HTTPParserState::ReadingHeadersReady ||
                        parser_state == HTTPParserState::Complete)
                        return WebSocketProcessingResult::Completed;

                    if (!strategy_read_write_until_socket_would_block)
                        return WebSocketProcessingResult::InProgress;
                    continue;
                }
                else if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                    return WebSocketProcessingResult::InProgress;
                else
                {
                    printf("[WebSocketConnection] Socket read error during handshake\n");
                    state = WebSocketConnectionState::Error;
                    return WebSocketProcessingResult::Error;
                }
            }

            return WebSocketProcessingResult::Error;
        }

        WebSocketProcessingResult WebSocketConnection::processHttpWriting()
        {
            while (true)
            {
                const uint8_t *data = http_writer->get_data();
                uint32_t data_size = http_writer->get_data_size();

                if (data == nullptr || data_size == 0)
                {
                    if (http_writer->state == HTTPWriterState::Complete)
                        return WebSocketProcessingResult::Completed;

                    printf("[WebSocketConnection] Logic error: writer returned null data\n");
                    state = WebSocketConnectionState::Error;
                    return WebSocketProcessingResult::Error;
                }

                uint32_t bytes_written = 0;
                uint32_t remaining = data_size - http_written_offset;
                Platform::SocketResult res = socket->write_buffer(data + http_written_offset, remaining, &bytes_written);

                if (res == Platform::SOCKET_RESULT_OK)
                {
                    http_written_offset += bytes_written;
                    if (http_written_offset >= data_size)
                    {
                        http_writer->next();
                        if (http_writer->state == HTTPWriterState::Error)
                        {
                            state = WebSocketConnectionState::Error;
                            return WebSocketProcessingResult::Error;
                        }
                        if (http_writer->state == HTTPWriterState::Complete)
                        {
                            return WebSocketProcessingResult::Completed;
                        }
                        http_written_offset = 0;
                    }

                    if (!strategy_read_write_until_socket_would_block)
                        return WebSocketProcessingResult::InProgress;
                    continue;
                }
                else if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                    return WebSocketProcessingResult::InProgress;
                else
                {
                    printf("[WebSocketConnection] Socket write error during handshake\n");
                    state = WebSocketConnectionState::Error;
                    return WebSocketProcessingResult::Error;
                }
            }

            return WebSocketProcessingResult::Error;
        }

        WebSocketProcessingResult WebSocketConnection::processServerHandshake()
        {
            // First, read the HTTP upgrade request
            if (http_parser->state != HTTPParserState::ReadingHeadersReady &&
                http_parser->state != HTTPParserState::Complete)
            {
                WebSocketProcessingResult result = processHttpReading();
                if (result != WebSocketProcessingResult::Completed)
                    return result;
            }

            // Headers are ready, now send response
            if (http_writer->state == HTTPWriterState::None)
            {
                if (!sendHandshakeResponse())
                    return WebSocketProcessingResult::Error;
            }

            // Write the response
            WebSocketProcessingResult result = processHttpWriting();
            if (result == WebSocketProcessingResult::Completed)
            {
                state = WebSocketConnectionState::Connected;
                reading_state = WebSocketReadingState::ReadingWaitingFrame;
                writing_state = WebSocketWritingState::WritingNone;
                frame_parser->reset();

                // free http parser/writer resources
                http_parser.reset();
                http_writer.reset();
                handshake_request.reset();
                handshake_response.reset();
            }

            return result;
        }

        WebSocketProcessingResult WebSocketConnection::processClientHandshake()
        {
            // First, write the HTTP upgrade request
            if (http_writer->state != HTTPWriterState::Complete)
            {
                WebSocketProcessingResult result = processHttpWriting();
                if (result != WebSocketProcessingResult::Completed)
                    return result;
            }

            // Request sent, now read response
            WebSocketProcessingResult result = processHttpReading();
            if (result != WebSocketProcessingResult::Completed)
                return result;

            // Validate response
            if (handshake_response->status_code != 101)
            {
                printf("[WebSocketConnection] Server rejected WebSocket upgrade: %d\n",
                       handshake_response->status_code);
                state = WebSocketConnectionState::Error;
                return WebSocketProcessingResult::Error;
            }

            std::string upgrade = handshake_response->getHeader("Upgrade");
            std::string connection = handshake_response->getHeader("Connection");
            std::string accept = handshake_response->getHeader("Sec-WebSocket-Accept");

            std::transform(upgrade.begin(), upgrade.end(), upgrade.begin(), ::tolower);
            std::transform(connection.begin(), connection.end(), connection.begin(), ::tolower);

            if (upgrade != "websocket" || connection.find("upgrade") == std::string::npos)
            {
                printf("[WebSocketConnection] Invalid upgrade response headers\n");
                state = WebSocketConnectionState::Error;
                return WebSocketProcessingResult::Error;
            }

            if (!clientValidateWebSocketAcceptKey(accept))
            {
                printf("[WebSocketConnection] Invalid Sec-WebSocket-Accept key\n");
                state = WebSocketConnectionState::Error;
                return WebSocketProcessingResult::Error;
            }

            // client generates the accept key and receives it from the server
            // websocket_key = ws_key;
            websocket_accept_key = accept;

            state = WebSocketConnectionState::Connected;
            reading_state = WebSocketReadingState::ReadingWaitingFrame;
            writing_state = WebSocketWritingState::WritingNone;
            frame_parser->reset();

            // free http parser/writer resources
            http_parser.reset();
            http_writer.reset();
            handshake_request.reset();
            handshake_response.reset();

            return WebSocketProcessingResult::Completed;
        }

        WebSocketProcessingResult WebSocketConnection::processReading()
        {
            if (reading_state == WebSocketReadingState::ReadingError)
                return WebSocketProcessingResult::Error;

            while (true)
            {
                // // First, process any remaining data
                // if (socket_buffer_remaining_size > 0)
                // {
                //     uint32_t bytes_parsed;
                //     WebSocketFrameParserState parser_state = frame_parser->insertData(
                //         socket_read_buffer_remaining.get(), socket_buffer_remaining_size, &bytes_parsed);

                //     if (parser_state == WebSocketFrameParserState::Error)
                //     {
                //         printf("[WebSocketConnection] Error parsing WebSocket frame\n");
                //         reading_state = WebSocketReadingState::ReadingError;
                //         state = WebSocketConnectionState::Error;
                //         return WebSocketProcessingResult::Error;
                //     }

                //     // Move unconsumed data to beginning
                //     if (bytes_parsed < socket_buffer_remaining_size)
                //     {
                //         memmove(socket_read_buffer_remaining.get(),
                //                 socket_read_buffer_remaining.get() + bytes_parsed,
                //                 socket_buffer_remaining_size - bytes_parsed);
                //     }
                //     socket_buffer_remaining_size -= bytes_parsed;

                //     if (parser_state == WebSocketFrameParserState::Complete)
                //     {
                //         // Create frame from parsed data
                //         auto frame = std::make_shared<WebSocketFrame>();
                //         frame->fin = frame_parser->isFin();
                //         frame->opcode = frame_parser->getOpcode();
                //         frame->payload = frame_parser->getPayload();

                //         incoming_frames.push(frame);
                //         reading_state = WebSocketReadingState::ReadingFrameComplete;
                //         frame_parser->reset();

                //         // Handle control frames automatically
                //         if (frame->isPing())
                //         {
                //             // Auto-respond with pong
                //             sendPong(std::string(frame->payload.begin(), frame->payload.end()));
                //         }
                //         else if (frame->isClose())
                //         {
                //             WebSocketCloseCode code;
                //             std::string reason;
                //             frame->getCloseInfo(&code, &reason);
                //             // Send close response if we haven't already
                //             if (state != WebSocketConnectionState::Closed)
                //             {
                //                 sendClose(code, reason);
                //             }
                //         }

                //         return WebSocketProcessingResult::Completed;
                //     }
                // }

                // Read more data from socket
                uint32_t bytes_read = 0;
                Platform::SocketResult res;
                if (socket_buffer_remaining_size > 0)
                {
                    res = Platform::SOCKET_RESULT_OK;
                    std::swap(socket_read_buffer_remaining, socket_read_buffer);
                    bytes_read = socket_buffer_remaining_size;
                    socket_buffer_remaining_size = 0;
                }
                else
                    res = socket->read_buffer(socket_read_buffer.get(), socket_buffer_size, &bytes_read);

                if (res == Platform::SOCKET_RESULT_OK)
                {
                    uint32_t bytes_parsed;
                    WebSocketFrameParserState parser_state = frame_parser->insertData(
                        socket_read_buffer.get(), bytes_read, &bytes_parsed);

                    if (parser_state == WebSocketFrameParserState::Error)
                    {
                        printf("[WebSocketConnection] Error parsing WebSocket frame\n");
                        reading_state = WebSocketReadingState::ReadingError;
                        state = WebSocketConnectionState::Error;
                        return WebSocketProcessingResult::Error;
                    }

                    // Save remaining data
                    if (bytes_parsed < bytes_read)
                    {
                        // uint32_t remaining = bytes_read - bytes_parsed;
                        // memcpy(socket_read_buffer_remaining.get() + socket_buffer_remaining_size,
                        //        socket_read_buffer.get() + bytes_parsed,
                        //        remaining);
                        // socket_buffer_remaining_size += remaining;
                        socket_buffer_remaining_size = bytes_read - bytes_parsed;
                        memcpy(socket_read_buffer_remaining.get(),
                               socket_read_buffer.get() + bytes_parsed,
                               socket_buffer_remaining_size);
                    }

                    if (parser_state == WebSocketFrameParserState::Complete)
                    {
                        // Create frame from parsed data
                        auto frame = std::make_shared<WebSocketFrame>();
                        frame->fin = frame_parser->isFin();
                        frame->opcode = frame_parser->getOpcode();
                        frame->payload = frame_parser->getPayload();

                        incoming_frames.push_back(frame);
                        reading_state = WebSocketReadingState::ReadingFrameComplete;
                        frame_parser->reset();

                        // Handle control frames automatically
                        if (frame->isPing())
                            sendPong(std::string(frame->payload.begin(), frame->payload.end()));
                        else if (frame->isClose())
                        {
                            WebSocketCloseCode code;
                            std::string reason;
                            frame->getCloseInfo(&code, &reason);
                            if (state != WebSocketConnectionState::Closed)
                                sendClose(code, reason);
                        }

                        return WebSocketProcessingResult::Completed;
                    }

                    if (!strategy_read_write_until_socket_would_block)
                        return WebSocketProcessingResult::InProgress;
                    continue;
                }
                else if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                {
                    reading_state = WebSocketReadingState::ReadingWaitingFrame;
                    return WebSocketProcessingResult::InProgress;
                }
                else if (res == Platform::SOCKET_RESULT_CLOSED)
                {
                    state = WebSocketConnectionState::Closed;
                    return WebSocketProcessingResult::Completed;
                }
                else
                {
                    printf("[WebSocketConnection] Socket read error\n");
                    reading_state = WebSocketReadingState::ReadingError;
                    state = WebSocketConnectionState::Error;
                    return WebSocketProcessingResult::Error;
                }
            }

            return WebSocketProcessingResult::InProgress;
        }

        WebSocketProcessingResult WebSocketConnection::processWriting()
        {
            Platform::AutoLock auto_lock(&writing_mutex);

            bool check_new_frames_for_writing = true;
            while (check_new_frames_for_writing)
            {
                check_new_frames_for_writing = false;

                if (writing_state == WebSocketWritingState::WritingError)
                    return WebSocketProcessingResult::Error;

                // Get next frame to write if we don't have one
                if (!current_write_frame && !outgoing_frames.empty())
                {
                    current_write_frame = outgoing_frames.front();
                    outgoing_frames.erase(outgoing_frames.begin());

                    // Client frames must be masked, server frames must not
                    bool mask = (mode == WebSocketConnectionMode::Client);
                    frame_writer->startFrame(current_write_frame->fin,
                                             current_write_frame->opcode,
                                             current_write_frame->payload.data(),
                                             current_write_frame->payload.size(),
                                             mask);
                    current_write_offset = 0;
                    writing_state = WebSocketWritingState::WritingFrame;
                }

                if (!current_write_frame)
                {
                    writing_state = WebSocketWritingState::WritingNone;
                    return WebSocketProcessingResult::Completed;
                }

                while (true)
                {
                    const uint8_t *data = frame_writer->getData();
                    uint32_t data_size = frame_writer->getDataSize();

                    if (data == nullptr || data_size == 0)
                    {
                        if (frame_writer->state == WebSocketFrameWriterState::Complete)
                        {
                            current_write_frame.reset();
                            writing_state = WebSocketWritingState::WritingFrameComplete;

                            // Check for more frames
                            if (!outgoing_frames.empty())
                            {
                                // return processWriting();
                                check_new_frames_for_writing = true;
                                break;
                            }

                            writing_state = WebSocketWritingState::WritingNone;
                            return WebSocketProcessingResult::Completed;
                        }

                        printf("[WebSocketConnection] Logic error: frame writer returned null data\n");
                        writing_state = WebSocketWritingState::WritingError;
                        state = WebSocketConnectionState::Error;
                        return WebSocketProcessingResult::Error;
                    }

                    uint32_t bytes_written = 0;
                    uint32_t remaining = data_size - current_write_offset;
                    Platform::SocketResult res = socket->write_buffer(data + current_write_offset, remaining, &bytes_written);

                    if (res == Platform::SOCKET_RESULT_OK)
                    {
                        current_write_offset += bytes_written;
                        if (current_write_offset >= data_size)
                        {
                            frame_writer->next();
                            if (frame_writer->state == WebSocketFrameWriterState::Error)
                            {
                                writing_state = WebSocketWritingState::WritingError;
                                state = WebSocketConnectionState::Error;
                                return WebSocketProcessingResult::Error;
                            }
                            if (frame_writer->state == WebSocketFrameWriterState::Complete)
                            {
                                current_write_frame.reset();
                                writing_state = WebSocketWritingState::WritingFrameComplete;

                                // Check for more frames
                                if (!outgoing_frames.empty())
                                {
                                    // return processWriting();
                                    check_new_frames_for_writing = true;
                                    break;
                                }

                                writing_state = WebSocketWritingState::WritingNone;
                                return WebSocketProcessingResult::Completed;
                            }
                            current_write_offset = 0;
                        }

                        if (!strategy_read_write_until_socket_would_block)
                            return WebSocketProcessingResult::InProgress;
                        continue;
                    }
                    else if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                        return WebSocketProcessingResult::InProgress;
                    else
                    {
                        printf("[WebSocketConnection] Socket write error\n");
                        writing_state = WebSocketWritingState::WritingError;
                        state = WebSocketConnectionState::Error;
                        return WebSocketProcessingResult::Error;
                    }
                }
                // if (check_new_frames_for_writing)
                //     continue;
            }

            return WebSocketProcessingResult::InProgress;
        }

        bool WebSocketConnection::sendWritingFrame(std::shared_ptr<WebSocketFrame> frame)
        {
            Platform::AutoLock auto_lock(&writing_mutex);

            if (state != WebSocketConnectionState::Connected &&
                state != WebSocketConnectionState::ProcessingConnection)
                return false;

            outgoing_frames.push_back(frame);
            state = WebSocketConnectionState::ProcessingConnection;
            return true;
        }

        bool WebSocketConnection::sendText(const std::string &text)
        {
            return sendWritingFrame(WebSocketFrame::CreateText(text));
        }

        bool WebSocketConnection::sendBinary(const std::vector<uint8_t> &data)
        {
            return sendWritingFrame(WebSocketFrame::CreateBinary(data));
        }

        bool WebSocketConnection::sendBinary(const uint8_t *data, size_t size)
        {
            return sendWritingFrame(WebSocketFrame::CreateBinary(data, size));
        }

        bool WebSocketConnection::sendPing(const std::string &data)
        {
            return sendWritingFrame(WebSocketFrame::CreatePing(data));
        }

        bool WebSocketConnection::sendPong(const std::string &data)
        {
            return sendWritingFrame(WebSocketFrame::CreatePong(data));
        }

        bool WebSocketConnection::sendClose(WebSocketCloseCode code, const std::string &reason)
        {
            return sendWritingFrame(WebSocketFrame::CreateClose(code, reason));
        }

        bool WebSocketConnection::hasReadingFrame() const
        {
            return !incoming_frames.empty();
        }

        std::shared_ptr<WebSocketFrame> WebSocketConnection::getReadingFrame()
        {
            if (incoming_frames.empty())
                return nullptr;

            auto frame = incoming_frames.front();
            incoming_frames.erase(incoming_frames.begin());

            if (incoming_frames.empty())
                reading_state = WebSocketReadingState::ReadingWaitingFrame;

            return frame;
        }

        bool WebSocketConnection::hasOutgoingFrames() const
        {
            Platform::AutoLock auto_lock(&writing_mutex);
            return !outgoing_frames.empty() || current_write_frame != nullptr;
        }

        WebSocketProcessingResult WebSocketConnection::process()
        {
            if (socket->isClosed())
            {
                state = WebSocketConnectionState::Closed;
                return WebSocketProcessingResult::Completed;
            }

            // Handle handshaking states
            if (state == WebSocketConnectionState::Server_Handshaking)
                return processServerHandshake();
            else if (state == WebSocketConnectionState::Client_Handshaking)
                return processClientHandshake();

            // Handle connected/processing states
            if (state == WebSocketConnectionState::Connected ||
                state == WebSocketConnectionState::ProcessingConnection)
            {
                state = WebSocketConnectionState::ProcessingConnection;

                // Process writing first (drain outgoing queue)
                if (hasOutgoingFrames())
                {
                    WebSocketProcessingResult write_result = processWriting();
                    if (write_result == WebSocketProcessingResult::Error)
                        return write_result;
                }

                // Process reading
                WebSocketProcessingResult read_result = processReading();
                if (read_result == WebSocketProcessingResult::Error)
                    return read_result;

                // Back to connected if no more activity
                if (!hasOutgoingFrames() && incoming_frames.empty())
                    state = WebSocketConnectionState::Connected;

                return WebSocketProcessingResult::InProgress;
            }

            if (state == WebSocketConnectionState::Error)
                return WebSocketProcessingResult::Error;

            if (state == WebSocketConnectionState::Closed)
                return WebSocketProcessingResult::Completed;

            return WebSocketProcessingResult::Completed;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif
