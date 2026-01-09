#include <InteractiveToolkit-Extension/network/HTTPBase.h>
#include <InteractiveToolkit/Platform/SocketTCP.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {
        bool HTTPBase::is_valid_header_character(char _chr)
        {
            return (_chr >= 32 && _chr <= 126) ||
                   _chr == '\r' ||
                   _chr == '\n' ||
                   _chr == '\t' ||
                   _chr == ' ';
        }

        bool HTTPBase::readFromStream(Platform::SocketTCP *socket, bool read_body_until_connection_close)
        {
            clear();

            bool first_line_readed = false;

            std::vector<char> line;
            line.reserve(HTTP_MAX_HEADER_RAW_SIZE);
            int max_http_header_lines = HTTP_MAX_HEADER_COUNT;

            char input_buffer[HTTP_READ_BUFFER_CHUNK_SIZE] = {0};
            uint32_t read_feedback = 0;
            uint32_t curr_reading = 0;
            bool found_crlf_alone = false;

            while (!found_crlf_alone)
            {
                if (!socket->read_buffer((uint8_t *)input_buffer, sizeof(input_buffer), &read_feedback))
                {
                    if (socket->isReadTimedout())
                    {
                        printf("[HTTP] Read NO Headers timed out\n");
                        return false;
                    }
                    else
                    {
                        printf("[HTTP] Connection or thread interrupted with the read feedback: %u\n", read_feedback);
                        return false;
                    }
                }

                curr_reading = 0;
                do
                {
                    bool found_header = false;

                    for (; curr_reading < read_feedback; curr_reading++)
                    {
                        if (!is_valid_header_character(input_buffer[curr_reading]))
                        {
                            printf("[HTTP] Invalid character in HTTP header: %u\n", (uint8_t)input_buffer[curr_reading]);
                            return false;
                        }
                        line.push_back(input_buffer[curr_reading]);
                        if (line.size() >= HTTP_MAX_HEADER_RAW_SIZE)
                        {
                            printf("[HTTP] HTTP line too long: %u\n", (uint32_t)line.size());
                            return false;
                        }

                        // CRLF check
                        if (line.size() >= 2 && line[line.size() - 1] == '\n' && line[line.size() - 2] == '\r')
                        {
                            // 2 situations: either a header line or a CRLF alone
                            found_header = line.size() > 2;
                            found_crlf_alone = !found_header;

                            // advance one position to keep the right byte count
                            curr_reading++;
                            break;
                        }
                    }

                    // header found
                    if (found_header)
                    {
                        std::string header = std::string(line.begin(), line.end() - 2);

                        // first header
                        if (!first_line_readed)
                        {
                            first_line_readed = true;
                            if (!read_first_line(header))
                                return false;
                        }
                        else
                        {
                            auto delimiter_pos = header.find(": ");
                            if (delimiter_pos != std::string::npos)
                            {
                                std::string key = header.substr(0, delimiter_pos);
                                std::string value = header.substr(delimiter_pos + 2);
                                headers[key] = value;
                            }
                        }

                        max_http_header_lines--;
                        if (max_http_header_lines <= 0)
                        {
                            printf("[HTTP] Too many HTTP header lines readed...\n");
                            return false;
                        }

                        line.clear();
                    }
                } while (!found_crlf_alone && curr_reading < read_feedback);
            } // end while !found_crlf_alone

            line.clear();

            // read body if Content-Length is set
            // HTTP headers are case-insensitive (RFC 7230)
            auto contentLengthHeader_it = findHeaderCaseInsensitive("Content-Length");
            auto transferEncoding_it = findHeaderCaseInsensitive("Transfer-Encoding");

            if (contentLengthHeader_it != headers.end())
            {
                uint32_t content_length;
                if (sscanf(contentLengthHeader_it->second.c_str(), "%u", &content_length) != 1)
                {
                    printf("[HTTPResponse] Invalid content_length code: %s\n", contentLengthHeader_it->second.c_str());
                    return false;
                }

                if (content_length == 0)
                    return true;
                else if (content_length >= HTTP_MAX_BODY_SIZE) // 100 MB limit
                    return false;

                body.resize(content_length);

                // copy content from curr_reading to body
                uint32_t already_read = (std::min)(read_feedback - curr_reading, content_length);
                if (already_read > 0)
                    memcpy(&body[0], &input_buffer[curr_reading], already_read);

                uint32_t total_read = already_read;
                while (total_read < content_length)
                {
                    uint32_t to_read = (std::min)(content_length - total_read, (uint32_t)HTTP_READ_BUFFER_CHUNK_SIZE);
                    if (!socket->read_buffer((uint8_t *)&body[total_read], to_read, &read_feedback))
                    {
                        if (socket->isReadTimedout())
                        {
                            printf("[HTTP] Read Body timed out\n");
                            return false;
                        }
                        else
                        {
                            printf("[HTTP] Connection or thread interrupted with the read feedback: %u\n", read_feedback);
                            return false;
                        }
                    }
                    total_read += read_feedback;
                }
            }
            else if (transferEncoding_it != headers.end() && transferEncoding_it->second.find("chunked") != std::string::npos)
            {
                // Read chunked encoding
                std::vector<char> chunk_buffer;
                chunk_buffer.reserve(HTTP_READ_BUFFER_CHUNK_SIZE);

                // Copy any remaining data from input_buffer
                for (uint32_t i = curr_reading; i < read_feedback; i++)
                    chunk_buffer.push_back(input_buffer[i]);

                bool finished = false;
                while (!finished)
                {
                    // Read chunk size line (hex number followed by \r\n)
                    while (true)
                    {
                        // Check if we have \r\n in buffer
                        bool found_crlf = false;
                        size_t crlf_pos = 0;
                        for (size_t i = 0; i + 1 < chunk_buffer.size(); i++)
                        {
                            if (chunk_buffer[i] == '\r' && chunk_buffer[i + 1] == '\n')
                            {
                                found_crlf = true;
                                crlf_pos = i;
                                break;
                            }
                        }

                        if (found_crlf)
                        {
                            // Parse chunk size
                            std::string chunk_size_str(chunk_buffer.begin(), chunk_buffer.begin() + crlf_pos);
                            uint32_t chunk_size = 0;
                            if (sscanf(chunk_size_str.c_str(), "%x", &chunk_size) != 1)
                            {
                                printf("[HTTP] Invalid chunk size: %s\n", chunk_size_str.c_str());
                                return false;
                            }

                            // printf("Chunk size: %u\n", chunk_size);

                            // Remove chunk size line from buffer
                            chunk_buffer.erase(chunk_buffer.begin(), chunk_buffer.begin() + crlf_pos + 2);

                            if (chunk_size == 0)
                            {
                                // Last chunk - finished
                                finished = true;
                                break;
                            }

                            // Read chunk data + \r\n
                            uint32_t total_chunk_data = chunk_size + 2; // +2 for trailing \r\n
                            while (chunk_buffer.size() < total_chunk_data)
                            {
                                uint32_t chunk_read = 0;
                                char temp_buffer[HTTP_READ_BUFFER_CHUNK_SIZE];
                                if (!socket->read_buffer((uint8_t *)temp_buffer, sizeof(temp_buffer), &chunk_read))
                                {
                                    if (socket->isReadTimedout())
                                    {
                                        printf("[HTTP] Read chunked data timed out\n");
                                        return false;
                                    }
                                    else
                                    {
                                        printf("[HTTP] Connection interrupted reading chunk\n");
                                        return false;
                                    }
                                }
                                chunk_buffer.insert(chunk_buffer.end(), temp_buffer, temp_buffer + chunk_read);
                            }

                            // Copy chunk data to body (excluding trailing \r\n)
                            body.insert(body.end(), chunk_buffer.begin(), chunk_buffer.begin() + chunk_size);

                            if (body.size() >= HTTP_MAX_BODY_SIZE)
                            {
                                printf("[HTTP] Body size exceeded limit during chunked read\n");
                                return false;
                            }

                            // Remove chunk data + \r\n from buffer
                            chunk_buffer.erase(chunk_buffer.begin(), chunk_buffer.begin() + total_chunk_data);
                            break;
                        }
                        else
                        {
                            // Need more data
                            uint32_t chunk_read = 0;
                            char temp_buffer[HTTP_READ_BUFFER_CHUNK_SIZE];
                            if (!socket->read_buffer((uint8_t *)temp_buffer, sizeof(temp_buffer), &chunk_read))
                            {
                                if (socket->isReadTimedout())
                                {
                                    printf("[HTTP] Read chunk size timed out\n");
                                    return false;
                                }
                                else
                                {
                                    printf("[HTTP] Connection interrupted reading chunk size\n");
                                    return false;
                                }
                            }
                            chunk_buffer.insert(chunk_buffer.end(), temp_buffer, temp_buffer + chunk_read);
                        }
                    }
                }
            }
            else if (read_body_until_connection_close)
            {
                // No Content-Length and no chunked encoding
                // Read until connection closes

                // Copy any remaining data from input_buffer
                for (uint32_t i = curr_reading; i < read_feedback; i++)
                    body.push_back(input_buffer[i]);

                while (true)
                {
                    uint32_t chunk_read = 0;
                    char temp_buffer[HTTP_READ_BUFFER_CHUNK_SIZE];
                    if (!socket->read_buffer((uint8_t *)temp_buffer, sizeof(temp_buffer), &chunk_read))
                    {
                        // Connection closed or error - this is expected for this case
                        break;
                    }

                    if (chunk_read == 0)
                        break; // EOF

                    body.insert(body.end(), temp_buffer, temp_buffer + chunk_read);

                    if (body.size() >= HTTP_MAX_BODY_SIZE)
                    {
                        printf("[HTTP] Body size exceeded limit reading until close\n");
                        return false;
                    }
                }
            }

            return true;
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
                headers.erase("Content-Length");
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
            if (!socket->write_buffer((uint8_t *)request_line.c_str(), (uint32_t)request_line.length(), &write_feedback))
                return false;

            // line ending
            const char *line_ending_crlf = "\r\n";
            if (!socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback))
                return false;

            for (const auto &header_pair : headers)
            {
                if (!socket->write_buffer((uint8_t *)header_pair.first.c_str(), (uint32_t)header_pair.first.length(), &write_feedback))
                    return false;

                const char *header_separator = ": ";
                if (!socket->write_buffer((uint8_t *)header_separator, 2, &write_feedback))
                    return false;

                if (!socket->write_buffer((uint8_t *)header_pair.second.c_str(), (uint32_t)header_pair.second.length(), &write_feedback))
                    return false;

                // const char *line_ending_crlf = "\r\n";
                if (!socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback))
                    return false;
            }

            // const char *line_ending_crlf = "\r\n";
            if (!socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback))
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
                        if (!socket->write_buffer((uint8_t *)chunk_header, (uint32_t)strlen(chunk_header), &write_feedback))
                            return false;

                        // Write chunk data
                        if (!socket->write_buffer((uint8_t *)&body[total_sent], chunk_size, &write_feedback))
                            return false;

                        // Write trailing \r\n
                        if (!socket->write_buffer((uint8_t *)line_ending_crlf, 2, &write_feedback))
                            return false;

                        total_sent += chunk_size;
                    }

                    // Write final chunk (size 0) to indicate end of chunked data
                    const char *final_chunk = "0\r\n\r\n";
                    if (!socket->write_buffer((uint8_t *)final_chunk, 5, &write_feedback))
                        return false;
                }
                else
                {
                    // Write body as normal (non-chunked)
                    if (!socket->write_buffer((uint8_t *)body.data(), (uint32_t)body.size(), &write_feedback))
                        return false;
                }
            }

            return true;
        }

        std::unordered_map<std::string, std::string>::const_iterator HTTPBase::findHeaderCaseInsensitive(const std::string &key) const
        {
            for (auto it = headers.begin(); it != headers.end(); ++it)
            {
                if (it->first.size() == key.size())
                {
                    bool match = true;
                    for (size_t i = 0; i < key.size(); i++)
                    {
                        if (std::tolower((unsigned char)it->first[i]) != std::tolower((unsigned char)key[i]))
                        {
                            match = false;
                            break;
                        }
                    }
                    if (match)
                        return it;
                }
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

        HTTPBase &HTTPBase::setHeader(const std::string &key,
                              const std::string &value)
        {
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
                headers.erase("Content-Type");
                headers.erase("Content-Length");
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
                headers.erase("Content-Type");
                headers.erase("Content-Length");
            }
            this->body.assign(body, body + body_size);
            return *this;
        }

        std::string HTTPBase::bodyAsString() const
        {
            return std::string(body.begin(), body.end());
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif