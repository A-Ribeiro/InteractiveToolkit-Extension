#include <InteractiveToolkit-Extension/network/HTTPConnection.h>
#include <InteractiveToolkit/Platform/SocketTCP.h>
#include <InteractiveToolkit/ITKCommon/StringUtil.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {
        static inline bool is_valid_http_header_str(const char *_header)
        {
            for (const char *p = _header; *p != '\0'; p++)
            {
                uint8_t ch = (uint8_t)(*p);
                if ((ch < 32 || ch > 126) && ch != '\r' && ch != '\n' && ch != '\t' && ch != ' ')
                    return false;
            }
            return true;
        }

        HTTPConnection::HTTPConnection(std::shared_ptr<Platform::SocketTCP> socket,
                                       HTTPConnectionMode mode,
                                       bool strategy_read_write_until_socket_would_block,
                                       uint32_t socket_buffer_size,
                                       uint32_t parser_buffer_size,
                                       uint32_t parser_max_header_count,
                                       uint32_t writer_buffer_size,
                                       uint32_t writing_transfer_encoding_max_size)
        {
            this->socket = socket;
            this->mode = mode;
            this->strategy_read_write_until_socket_would_block = strategy_read_write_until_socket_would_block;
            this->socket_buffer_size = socket_buffer_size;
            this->parser_buffer_size = parser_buffer_size;
            this->parser_max_header_count = parser_max_header_count;
            this->writer_buffer_size = writer_buffer_size;
            this->writing_transfer_encoding_max_size = writing_transfer_encoding_max_size;
            this->socket_read_buffer = STL_Tools::make_unique<uint8_t[]>(socket_buffer_size);

            this->socket_buffer_body_init_size = 0;
            this->socket_read_buffer_body_init = STL_Tools::make_unique<uint8_t[]>(socket_buffer_size);

            this->written_offset = 0;

            parser = STL_Tools::make_unique<HTTPParser>(parser_buffer_size, parser_max_header_count);
            writer = STL_Tools::make_unique<HTTPWriter>(writer_buffer_size);

            if (mode == HTTPConnectionMode::Server)
            {
                request = HTTPRequestAsync::CreateShared();
                parser->initialize(false, request->getHTTPParserCallbacks());
                state = HTTPConnectionState::Server_ReadingRequestHeaders;
            }
            else
            {
                response = HTTPResponseAsync::CreateShared();
                // parser->initialize(true, response->getHTTPParserCallbacks());
                state = HTTPConnectionState::Client_WritingRequest;
            }
        }

        HTTPConnection::~HTTPConnection()
        {
            close();
        }

        HTTPProcessingResult HTTPConnection::processReading()
        {
            // this call checks for state transitions without data
            parser->checkOnlyStateTransition();
            if (parser->state == HTTPParserState::Complete)
            {
                state = (mode == HTTPConnectionMode::Server) ? HTTPConnectionState::Server_ReadingRequestBodyComplete
                                                             : HTTPConnectionState::Client_ReadingResponseBodyComplete;
                return HTTPProcessingResult::Completed;
            }

            while (true)
            {
                uint32_t bytes_read = 0;
                Platform::SocketResult res;
                if (state == HTTPConnectionState::Server_ReadingRequestBody && socket_buffer_body_init_size > 0)
                {
                    res = Platform::SOCKET_RESULT_OK;
                    std::swap(socket_read_buffer_body_init, socket_read_buffer);
                    bytes_read = socket_buffer_body_init_size;
                    socket_buffer_body_init_size = 0;
                }
                else
                    res = socket->read_buffer(socket_read_buffer.get(), socket_buffer_size, &bytes_read);

                if (res == Platform::SOCKET_RESULT_OK)
                {
                    uint32_t total_bytes_parsed = 0;
                    while (total_bytes_parsed < bytes_read)
                    {
                        uint32_t bytes_parsed;
                        if (parser->insertData(socket_read_buffer.get(), bytes_read, &bytes_parsed) == HTTPParserState::Error)
                        {
                            printf("[HTTPConnection] Error parsing HTTP stream\n");
                            state = HTTPConnectionState::Error;
                            return HTTPProcessingResult::Error;
                        }
                        if (parser->state == HTTPParserState::ReadingHeadersReady)
                        {
                            // save init body data if any
                            socket_buffer_body_init_size = bytes_read - bytes_parsed;
                            if (socket_buffer_body_init_size > 0)
                                memcpy(socket_read_buffer_body_init.get(),
                                       socket_read_buffer.get() + bytes_parsed,
                                       socket_buffer_body_init_size);

                            state = (mode == HTTPConnectionMode::Server) ? HTTPConnectionState::Server_ReadingRequestHeadersComplete
                                                                         : HTTPConnectionState::Client_ReadingResponseHeadersComplete;
                            return HTTPProcessingResult::InProgress;
                        }
                        if (parser->state == HTTPParserState::Complete)
                        {
                            state = (mode == HTTPConnectionMode::Server) ? HTTPConnectionState::Server_ReadingRequestBodyComplete
                                                                         : HTTPConnectionState::Client_ReadingResponseBodyComplete;
                            return HTTPProcessingResult::Completed;
                        }
                        total_bytes_parsed += bytes_parsed;
                    }

                    // Need more data
                    if (!strategy_read_write_until_socket_would_block)
                        return HTTPProcessingResult::InProgress;
                    else
                        continue;
                }
                else if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                    return HTTPProcessingResult::InProgress;
                else if (res == Platform::SOCKET_RESULT_CLOSED)
                {
                    parser->connectionClosed();
                    if (parser->state == HTTPParserState::Complete)
                    {
                        state = (mode == HTTPConnectionMode::Server) ? HTTPConnectionState::Server_ReadingRequestBodyComplete
                                                                     : HTTPConnectionState::Client_ReadingResponseBodyComplete;
                        return HTTPProcessingResult::Completed;
                    }
                    else
                    {
                        state = HTTPConnectionState::Error;
                        return HTTPProcessingResult::Error;
                    }
                }
                else
                {
                    printf("[HTTPConnection] Socket read error\n");
                    state = HTTPConnectionState::Error;
                    return HTTPProcessingResult::Error;
                }
            }

            printf("[HTTPConnection] Abnormal reading error\n");
            state = HTTPConnectionState::Error;
            return HTTPProcessingResult::Error;
        }

        HTTPProcessingResult HTTPConnection::processWriting()
        {
            while (true)
            {
                const uint8_t *data = writer->get_data();
                uint32_t data_size = writer->get_data_size();

                if (data == nullptr || data_size == 0)
                {
                    printf("[HTTPConnection] Logic error: writer returned null data or zero size\n");
                    state = HTTPConnectionState::Error;
                    return HTTPProcessingResult::Error;
                }

                uint32_t bytes_written = 0;
                uint32_t remaining = (uint32_t)(data_size - written_offset);
                Platform::SocketResult res = socket->write_buffer(data + written_offset, remaining, &bytes_written);
                // Platform::Sleep::millis(2000); // wait a bit for client to send data

                if (res == Platform::SOCKET_RESULT_OK)
                {
                    written_offset += bytes_written;
                    if (written_offset >= data_size)
                    {
                        // Advance writer state
                        writer->next();
                        if (writer->state == HTTPWriterState::Error)
                        {
                            state = HTTPConnectionState::Error;
                            return HTTPProcessingResult::Error;
                        }
                        if (writer->state == HTTPWriterState::Complete)
                        {
                            state = (mode == HTTPConnectionMode::Server) ? HTTPConnectionState::Server_WritingResponseComplete
                                                                         : HTTPConnectionState::Client_WritingRequestComplete;
                            return HTTPProcessingResult::Completed;
                        }
                        written_offset = 0;
                    }
                    // Could write more data
                    if (!strategy_read_write_until_socket_would_block)
                        return HTTPProcessingResult::InProgress;
                    else
                        continue;
                }
                else if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                    return HTTPProcessingResult::InProgress;
                else
                {
                    printf("[HTTPConnection] Socket write error\n");
                    state = HTTPConnectionState::Error;
                    return HTTPProcessingResult::Error;
                }
            }

            printf("[HTTPConnection] Abnormal writing error\n");
            state = HTTPConnectionState::Error;
            return HTTPProcessingResult::Error;
        }

        HTTPProcessingResult HTTPConnection::process()
        {
            if (socket->isClosed())
            {
                printf("[HTTPConnection] Socket is closed\n");
                state = HTTPConnectionState::Error;
                return HTTPProcessingResult::Error;
            }

            if (state == HTTPConnectionState::Server_ReadingRequestHeaders ||
                state == HTTPConnectionState::Server_ReadingRequestBody ||
                state == HTTPConnectionState::Client_ReadingResponseHeaders ||
                state == HTTPConnectionState::Client_ReadingResponseBody)
                return processReading();
            else if (state == HTTPConnectionState::Server_WritingResponse ||
                     state == HTTPConnectionState::Client_WritingRequest)
                return processWriting();

            if (state == HTTPConnectionState::Error)
                return HTTPProcessingResult::Error;

            if (state == HTTPConnectionState::Closed)
                return HTTPProcessingResult::Completed;

            return HTTPProcessingResult::Completed;
        }

        void HTTPConnection::close()
        {
            socket->close();
            state = HTTPConnectionState::Closed;
        }

        bool HTTPConnection::validateBeforeWrite(std::shared_ptr<HTTPBaseAsync> http_obj, uint32_t _writing_transfer_encoding_max_size)
        {
            if (!is_valid_http_header_str(http_obj->firstLine().c_str()))
            {
                printf("[HTTPConnection] Invalid characters in HTTP first line: '%s'\n",
                       http_obj->firstLine().c_str());
                return false;
            }
            for (const auto &header : http_obj->listHeaders())
            {
                if (!is_valid_http_header_str(header.first.c_str()) ||
                    !is_valid_http_header_str(header.second.c_str()))
                {
                    printf("[HTTPConnection] Invalid HTTP header: '%s: %s'\n",
                           header.first.c_str(), header.second.c_str());
                    return false;
                }
            }

            // check for chunked encoding and content-length consistency
            auto body_provider = http_obj->getBodyProvider();
            int body_size = body_provider->body_read_get_size();

            bool must_be_chunked = (body_size < 0) || (body_size > (int)_writing_transfer_encoding_max_size);
            bool set_as_chunked = http_obj->getHeader("Transfer-Encoding").find("chunked") != std::string::npos;
            if (must_be_chunked && !set_as_chunked)
            {
                http_obj->setHeader("Transfer-Encoding", "chunked");
                set_as_chunked = true;
            }
            if (set_as_chunked)
                http_obj->eraseHeader("Content-Length");
            else if (body_size >= 0)
            {
                // not set as chunked, set content-length
                std::string content_length_str = std::to_string(body_size);
                if (http_obj->getHeader("Content-Length") != content_length_str)
                    http_obj->setHeader("Content-Length", content_length_str);
            }
            else
            {
                printf("[HTTPConnection] Body size unknown but Transfer-Encoding is not chunked\n");
                return false;
            }
http_obj->eraseHeader("Content-Length");
            return true;
        }

        bool HTTPConnection::serverContinueBodyReading()
        {
            if (mode != HTTPConnectionMode::Server)
            {
                printf("[HTTPConnection] Invalid operation: serverContinueBodyReading called in client mode\n");
                state = HTTPConnectionState::Error;
                return false;
            }
            if (state != HTTPConnectionState::Server_ReadingRequestHeadersComplete)
            {
                printf("[HTTPConnection] Invalid operation: serverContinueBodyReading called before completing request reading headers\n");
                state = HTTPConnectionState::Error;
                return false;
            }
            parser->headersReadyApplyNextBodyState(socket_buffer_body_init_size);
            state = HTTPConnectionState::Server_ReadingRequestBody;
            return true;
        }

        bool HTTPConnection::serverBeginWriteResponse(std::shared_ptr<HTTPResponseAsync> response)
        {
            if (mode != HTTPConnectionMode::Server)
            {
                printf("[HTTPConnection] Invalid operation: serverBeginWriteResponse called in client mode\n");
                state = HTTPConnectionState::Error;
                return false;
            }
            if (state != HTTPConnectionState::Server_ReadingRequestBodyComplete &&
                state != HTTPConnectionState::Server_ReadingRequestHeadersComplete)
            {
                printf("[HTTPConnection] Invalid operation: serverBeginWriteResponse called before completing request reading\n");
                state = HTTPConnectionState::Error;
                return false;
            }

            this->response = response;
            this->writer->initialize(writing_transfer_encoding_max_size, response->getHTTPWriterCallbacks());
            state = HTTPConnectionState::Server_WritingResponse;
            if (!validateBeforeWrite(response, writing_transfer_encoding_max_size))
            {
                state = HTTPConnectionState::Error;
                return false;
            }

            written_offset = 0;
            this->writer->start_streaming();
            if (writer->state == HTTPWriterState::Error)
            {
                state = HTTPConnectionState::Error;
                return false;
            }
            if (writer->state == HTTPWriterState::Complete)
            {
                state = HTTPConnectionState::Server_WritingResponseComplete;
                return true;
            }

            return true;
        }

        bool HTTPConnection::clientBeginWriteRequest(std::shared_ptr<HTTPRequestAsync> request)
        {
            if (mode != HTTPConnectionMode::Client)
            {
                printf("[HTTPConnection] Invalid operation: clientBeginWriteRequest called in server mode\n");
                state = HTTPConnectionState::Error;
                return false;
            }
            if (state != HTTPConnectionState::Client_WritingRequest)
            {
                printf("[HTTPConnection] Invalid operation: clientBeginWriteRequest called in invalid state\n");
                state = HTTPConnectionState::Error;
                return false;
            }

            this->request = request;
            this->writer->initialize(writing_transfer_encoding_max_size, request->getHTTPWriterCallbacks());
            state = HTTPConnectionState::Client_WritingRequest;
            if (!validateBeforeWrite(request, writing_transfer_encoding_max_size))
            {
                state = HTTPConnectionState::Error;
                return false;
            }

            written_offset = 0;
            this->writer->start_streaming();
            if (writer->state == HTTPWriterState::Error)
            {
                state = HTTPConnectionState::Error;
                return false;
            }
            if (writer->state == HTTPWriterState::Complete)
            {
                state = HTTPConnectionState::Client_WritingRequestComplete;
                return true;
            }

            return true;
        }

        bool HTTPConnection::clientBeginReadResponseHeaders()
        {
            if (mode != HTTPConnectionMode::Client)
            {
                printf("[HTTPConnection] Invalid operation: clientBeginReadResponse called in server mode\n");
                state = HTTPConnectionState::Error;
                return false;
            }
            if (state != HTTPConnectionState::Client_WritingRequestComplete)
            {
                printf("[HTTPConnection] Invalid operation: clientBeginReadResponse called before completing request writing\n");
                state = HTTPConnectionState::Error;
                return false;
            }

            parser->initialize(true, response->getHTTPParserCallbacks());
            state = HTTPConnectionState::Client_ReadingResponseHeaders;
            socket_buffer_body_init_size = 0;
            return true;
        }

        bool HTTPConnection::clientContinueBodyReading()
        {
            if (mode != HTTPConnectionMode::Client)
            {
                printf("[HTTPConnection] Invalid operation: clientContinueBodyReading called in server mode\n");
                state = HTTPConnectionState::Error;
                return false;
            }
            if (state != HTTPConnectionState::Client_ReadingResponseHeadersComplete)
            {
                printf("[HTTPConnection] Invalid operation: clientContinueBodyReading called before completing response reading headers\n");
                state = HTTPConnectionState::Error;
                return false;
            }
            parser->headersReadyApplyNextBodyState(socket_buffer_body_init_size);
            state = HTTPConnectionState::Client_ReadingResponseBody;
            return true;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif
