#include <InteractiveToolkit-Extension/network/HTTPBase.h>
#include <InteractiveToolkit/Platform/SocketTCP.h>
#include <InteractiveToolkit/AlgorithmCore/PatternMatch/array_index_of.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {
        static inline bool is_valid_header_character(char _chr)
        {
            return (_chr >= 32 && _chr <= 126) ||
                   _chr == '\r' ||
                   _chr == '\n' ||
                   _chr == '\t' ||
                   _chr == ' ';
        }

        static inline int strcasecmp_custom(const char *s1, const char *s2)
        {
#if defined(_WIN32)
            return _stricmp(s1, s2);
#else
            return strcasecmp(s1, s2);
#endif
        }

        bool HTTPBase::readFromStream(Platform::SocketTCP *socket, bool read_body_until_connection_close)
        {
            clear();

            HTTPParser parser(HTTP_READ_BUFFER_CHUNK_SIZE, HTTP_MAX_HEADER_COUNT);
            HTTPStreamCallbacks callbacks{
                // onFirstLine
                [this](const char *main_header)
                {
                    return this->read_first_line(std::string(main_header));
                },
                // onHeader
                [this](const char *key, const char *value)
                {
                    this->headers[std::string(key)] = std::string(value);
                    return true;
                },
                // onHeadersComplete
                [](const uint8_t *remaining_data, uint32_t size)
                {
                    // no special action needed
                    return true;
                },
                // onBodyPart
                [this](const uint8_t *data, uint32_t size)
                {
                    if (this->body.size() + size > HTTP_MAX_BODY_SIZE)
                    {
                        printf("[HTTP] Body size exceeds maximum allowed size: %u bytes\n", HTTP_MAX_BODY_SIZE);
                        return false;
                    }
                    this->body.insert(this->body.end(), data, data + size);
                    return true;
                },
                // onComplete
                []()
                {
                    return true;
                }};
            parser.initialize(read_body_until_connection_close, callbacks);

            uint8_t reading_buffer[HTTP_READ_BUFFER_CHUNK_SIZE];
            uint32_t bytes_read = 0;
            while (parser.state != HTTPParserState::Complete)
            {
                Platform::SocketResult res = socket->read_buffer(reading_buffer, sizeof(reading_buffer), &bytes_read);
                if (res != Platform::SOCKET_RESULT_OK)
                {
                    if (res == Platform::SOCKET_RESULT_CLOSED)
                    {
                        parser.connectionClosed();
                        break;
                    }
                    printf("[HTTP] Error reading from socket\n");
                    return false;
                }
                if (parser.insertData(reading_buffer, bytes_read) == HTTPParserState::Error)
                {
                    printf("[HTTP] Error parsing HTTP stream data\n");
                    return false;
                }
            }

            if (parser.state != HTTPParserState::Complete)
            {
                printf("[HTTP] Error parsing HTTP stream\n");
                return false;
            }

            return true;

            // std::vector<uint8_t> input_buffer(HTTP_READ_BUFFER_CHUNK_SIZE * 2);
            // return parseHTTPStream(
            //     socket,
            //     {// onFirstLine
            //      [this](const char *main_header)
            //      {
            //          return this->read_first_line(std::string(main_header));
            //      },
            //      // onHeader
            //      [this](const char *key, const char *value)
            //      {
            //             this->headers[std::string(key)] = std::string(value);
            //             return true; },
            //      // onHeadersComplete
            //      [](const uint8_t *remaining_data, uint32_t size)
            //      {
            //             // no special action needed
            //             return true; },
            //      // onBodyPart
            //      [this](const uint8_t *data, uint32_t size)
            //      {
            //         if (this->body.size() + size > HTTP_MAX_BODY_SIZE)
            //         {
            //             printf("[HTTP] Body size exceeds maximum allowed size: %u bytes\n", HTTP_MAX_BODY_SIZE);
            //             return false;
            //         }
            //         this->body.insert(this->body.end(), data, data + size);
            //         return true; },
            //      // onComplete
            //      []()
            //      { return true; }},
            //     input_buffer.data(),                               // input buffer_a
            //     input_buffer.data() + HTTP_READ_BUFFER_CHUNK_SIZE, // input buffer_b
            //     (uint32_t)HTTP_READ_BUFFER_CHUNK_SIZE,             // input buffer size
            //     HTTP_MAX_HEADER_RAW_SIZE,                          // max_header_size_bytes
            //     HTTP_MAX_HEADER_COUNT,                             // max header count
            //     read_body_until_connection_close                   // read body until connection close
            // );
        }

        bool HTTPBase::writeToStream(Platform::SocketTCP *socket)
        {
            // check headers validity
            for (const auto &header_pair : headers)
            {
                for (const char &ch : header_pair.first)
                {
                    if (!is_valid_header_character(ch))
                    {
                        printf("[HTTP] Invalid character in HTTP request line: %u\n", (uint8_t)ch);
                        return false;
                    }
                }
                for (const char &ch : header_pair.second)
                {
                    if (!is_valid_header_character(ch))
                    {
                        printf("[HTTP] Invalid character in HTTP request line: %u\n", (uint8_t)ch);
                        return false;
                    }
                }
            }

            // Determine if we need chunked transfer encoding
            bool use_chunked_encoding = body.size() > HTTP_TRANSFER_ENCODING_MAX_SIZE;

            if (use_chunked_encoding)
            {
                // Remove Content-Length and add Transfer-Encoding: chunked
                eraseHeader("Content-Length");
                headers["Transfer-Encoding"] = "chunked";
            }
            else if (body.size() > 0)
            {
                if (getHeader("Content-Length") != std::to_string(body.size()))
                {
                    printf("[HTTP] Content-Length header does not match body size\n");
                    return false;
                }
            }
            else if (hasHeader("Content-Length"))
            {
                printf("[HTTP] Content-Length header set but body is empty\n");
                return false;
            }

            std::string request_line = mount_first_line();
            for (const char &ch : request_line)
            {
                if (!is_valid_header_character(ch))
                {
                    printf("[HTTP] Invalid character in HTTP request line: %u\n", (uint8_t)ch);
                    return false;
                }
            }

            uint32_t write_feedback = 0;
            if (socket->write_buffer((uint8_t *)request_line.c_str(), (uint32_t)request_line.length(), &write_feedback) != Platform::SOCKET_RESULT_OK)
                return false;

            // line ending
            const char *line_ending_crlf = "\r\n";
            if (socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback) != Platform::SOCKET_RESULT_OK)
                return false;

            for (const auto &header_pair : headers)
            {
                if (socket->write_buffer((uint8_t *)header_pair.first.c_str(), (uint32_t)header_pair.first.length(), &write_feedback) != Platform::SOCKET_RESULT_OK)
                    return false;

                const char *header_separator = ": ";
                if (socket->write_buffer((uint8_t *)header_separator, 2, &write_feedback) != Platform::SOCKET_RESULT_OK)
                    return false;

                if (socket->write_buffer((uint8_t *)header_pair.second.c_str(), (uint32_t)header_pair.second.length(), &write_feedback) != Platform::SOCKET_RESULT_OK)
                    return false;

                // const char *line_ending_crlf = "\r\n";
                if (socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback) != Platform::SOCKET_RESULT_OK)
                    return false;
            }

            // const char *line_ending_crlf = "\r\n";
            if (socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback) != Platform::SOCKET_RESULT_OK)
                return false;

            // body
            if (body.size() > 0)
            {
                if (use_chunked_encoding)
                {
                    // Write body using chunked transfer encoding
                    uint32_t total_sent = 0;
                    while (total_sent < body.size())
                    {
                        uint32_t chunk_size = (std::min)((uint32_t)HTTP_TRANSFER_ENCODING_MAX_SIZE, (uint32_t)(body.size() - total_sent));

                        // Write chunk size in hexadecimal followed by \r\n
                        char chunk_header[32];
                        snprintf(chunk_header, sizeof(chunk_header), "%x\r\n", chunk_size);
                        if (socket->write_buffer((uint8_t *)chunk_header, (uint32_t)strlen(chunk_header), &write_feedback) != Platform::SOCKET_RESULT_OK)
                            return false;

                        // Write chunk data
                        if (socket->write_buffer((uint8_t *)&body[total_sent], chunk_size, &write_feedback) != Platform::SOCKET_RESULT_OK)
                            return false;

                        // Write trailing \r\n
                        if (socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback) != Platform::SOCKET_RESULT_OK)
                            return false;

                        total_sent += chunk_size;
                    }

                    // Write final chunk (size 0) to indicate end of chunked data
                    const char *final_chunk = "0\r\n\r\n";
                    if (socket->write_buffer((uint8_t *)final_chunk, 5, &write_feedback) != Platform::SOCKET_RESULT_OK)
                        return false;
                }
                else
                {
                    // Write body as normal (non-chunked)
                    if (socket->write_buffer((uint8_t *)body.data(), (uint32_t)body.size(), &write_feedback) != Platform::SOCKET_RESULT_OK)
                        return false;
                }
            }

            return true;
        }

        std::unordered_map<std::string, std::string>::const_iterator HTTPBase::findHeaderCaseInsensitive(const std::string &key) const
        {
            for (auto it = headers.begin(); it != headers.end(); ++it)
            {
                if (it->first.size() == key.size() && strcasecmp_custom(it->first.c_str(), key.c_str()) == 0)
                    return it;
            }
            return headers.end();
        }

        const std::unordered_map<std::string, std::string> &HTTPBase::listHeaders() const
        {
            return headers;
        }

        bool HTTPBase::hasHeader(const std::string &key) const
        {
            return findHeaderCaseInsensitive(key) != headers.end();
        }

        std::string HTTPBase::getHeader(const std::string &key) const
        {
            auto it = findHeaderCaseInsensitive(key);
            if (it != headers.end())
                return it->second;
            return "";
        }

        void HTTPBase::eraseHeader(const std::string &key)
        {
            auto it = findHeaderCaseInsensitive(key);
            if (it != headers.end())
                headers.erase(it);
        }

        HTTPBase &HTTPBase::setHeader(const std::string &key,
                                      const std::string &value)
        {
            if (value.length() == 0)
            { // erase header if value is empty
                eraseHeader(key);
                return *this;
            }
            std::string key_copy;
            key_copy.resize(key.length());
            for (int i = 0; i < (int)key.length(); i++)
                key_copy[i] = (!is_valid_header_character(key[i])) ? ' ' : key[i];
            std::string value_copy;
            value_copy.resize(value.length());
            for (int i = 0; i < (int)value.length(); i++)
                value_copy[i] = (!is_valid_header_character(value[i])) ? ' ' : value[i];
            headers[key_copy] = value_copy;
            return *this;
        }

        HTTPBase &HTTPBase::setBody(const std::string &body,
                                    const std::string &content_type)
        {
            if (body.size() > 0)
            {
                headers["Content-Type"] = content_type;
                headers["Content-Length"] = std::to_string(body.size());
            }
            else
            {
                eraseHeader("Content-Type");
                eraseHeader("Content-Length");
            }
            this->body = std::vector<uint8_t>(body.begin(), body.end());
            return *this;
        }

        HTTPBase &HTTPBase::setBody(const uint8_t *body, uint32_t body_size,
                                    const std::string &content_type)
        {
            if (body_size > 0)
            {
                headers["Content-Type"] = content_type;
                headers["Content-Length"] = std::to_string(body_size);
            }
            else
            {
                eraseHeader("Content-Type");
                eraseHeader("Content-Length");
            }
            this->body.assign(body, body + body_size);
            return *this;
        }

        std::string HTTPBase::bodyAsString() const
        {
            return std::string(body.begin(), body.end());
        }

        const std::vector<uint8_t> &HTTPBase::bodyAsVector() const
        {
            return body;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif