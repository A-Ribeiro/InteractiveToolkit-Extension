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

        static inline void *memcpy_custom(uint8_t *dst, const uint8_t *src, uint32_t size)
        {
            if (size == 0)
                return dst;
            // non overlapping buffers
            if (src > dst + size || dst > src + size)
                return memcpy(dst, src, size);
            // overlapping buffers, copy byte by byte
            for (uint32_t i = 0; i < size; i++)
                dst[i] = src[i];
            return dst;
        }

        static inline bool checkContentLengthHeader(const char *key, const char *value, uint32_t *content_length)
        {
            if (strcasecmp_custom(key, "Content-Length") != 0)
                return false;

            if (sscanf(value, "%u", content_length) != 1)
            {
                printf("[HTTPResponse] Invalid content_length code: %s\n", value);
                return false;
            }

            return true;
        }

        static inline bool checkTransferEncodingHeader(const char *key, const char *value, bool *is_chunked)
        {
            if (strcasecmp_custom(key, "Transfer-Encoding") != 0)
                return false;

            const char *pos = strstr(value, "chunked");
            // if (pos)
            //     index = pos - value; // offset from start
            *is_chunked = pos != nullptr;

            return true;
        }

        bool parseHTTPStream(Platform::SocketTCP *socket,
                             const HTTPStreamCallbacks &callbacks,
                             uint8_t *input_buffer, uint32_t input_buffer_size,
                             uint32_t max_header_size_bytes,
                             uint32_t max_header_count,
                             bool read_body_until_connection_close)
        {
            using namespace AlgorithmCore::PatternMatch;

            if (input_buffer_size < max_header_size_bytes + 2)
            {
                printf("[HTTP] max header size + 2 (CRLF) must fit into input buffer size.\n");
                return false;
            }

            // reading headers
            uint32_t input_buffer_start = 0;
            uint32_t input_buffer_end = 0;
            bool first_line = true;

            // body special headers
            uint32_t content_length = 0;
            bool is_chunked = false;

            bool content_length_present = false;
            bool transfer_encoding_present = false;

            // parse headers data
            while (true)
            {
                const char *pattern = "\r\n";
                // Only need to check 1 byte back for boundary case where '\r' is at read_check_start-1
                // and '\n' is at read_check_start (pattern spans old/new data boundary)
                uint32_t start_buffer_search = 0;
                if (input_buffer_start >= 1)
                    start_buffer_search = input_buffer_start - 1;
                uint32_t crlf_pos = array_index_of(input_buffer, start_buffer_search, input_buffer_end, (const uint8_t *)pattern, 2);
                bool found_crlf = crlf_pos < input_buffer_end;

                if (found_crlf)
                {
                    if (crlf_pos > max_header_size_bytes)
                    {
                        printf("[HTTP] HTTP header too large: %u bytes\n", (uint32_t)(crlf_pos + 2));
                        return false;
                    }

                    input_buffer[crlf_pos] = '\0';
                    // check header bytes
                    for (uint32_t i = 0; i < crlf_pos; i++)
                    {
                        if (!is_valid_header_character(input_buffer[i]))
                        {
                            printf("[HTTP] Invalid character in HTTP header: %u\n", (uint8_t)input_buffer[i]);
                            return false;
                        }
                    }

                    if (first_line)
                    {
                        if (!callbacks.onFirstLine((const char *)input_buffer))
                            return false;
                        first_line = false;
                    }
                    else if (crlf_pos > 0)
                    {
                        // normal header
                        const char *pattern_header = ": ";
                        size_t value_pos = array_index_of(input_buffer, 0, crlf_pos, (const uint8_t *)pattern_header, 2);
                        bool found_key_value = value_pos < crlf_pos;
                        if (!found_key_value)
                        {
                            printf("[HTTP] Invalid HTTP header format: %s\n", (const char *)input_buffer);
                            return false;
                        }

                        input_buffer[value_pos] = '\0';
                        const char *key = (const char *)input_buffer;
                        const char *value = (const char *)&input_buffer[value_pos + 2];

                        if (!content_length_present)
                            content_length_present |= checkContentLengthHeader(key, value, &content_length);
                        if (!transfer_encoding_present)
                            transfer_encoding_present |= checkTransferEncodingHeader(key, value, &is_chunked);

                        if (!callbacks.onHeader(key, value))
                            return false;

                        max_header_count--;
                        if (max_header_count == 0)
                        {
                            printf("[HTTP] Too many HTTP header lines readed...\n");
                            return false;
                        }
                    }

                    // move the remaining data to the beginning of the buffer
                    uint32_t line_ending_pos = crlf_pos + 2;
                    input_buffer_end -= line_ending_pos;
                    input_buffer_start = 0;
                    memcpy_custom(input_buffer, &input_buffer[line_ending_pos], input_buffer_end);

                    if (crlf_pos == 0)
                    {
                        // headers ending detection
                        if (!callbacks.onHeadersComplete(input_buffer, input_buffer_end))
                            return false;
                        break; // end of headers
                    }
                }
                else
                {
                    // need more data
                    input_buffer_start = input_buffer_end;
                    if (input_buffer_end >= input_buffer_size)
                    {
                        printf("[HTTP] header parsing overflow, increase the HTTP_MAX_HEADER_RAW_SIZE constant.\n");
                        return false;
                    }
                    if (input_buffer_start > max_header_size_bytes)
                    {
                        printf("[HTTP] HTTP header too large: %u bytes\n", input_buffer_start);
                        return false;
                    }
                    uint32_t readed_feedback;
                    if (!socket->read_buffer(input_buffer + input_buffer_end, input_buffer_size - input_buffer_end, &readed_feedback))
                    {
                        if (socket->isReadTimedout())
                            printf("[HTTP] Socket read timed out\n");
                        else
                            printf("[HTTP] Connection or thread interrupted with the read feedback: %u (must be 0)\n", input_buffer_end);
                        return false;
                    }
                    input_buffer_end += readed_feedback;
                }
            }

            // parse body data
            if (transfer_encoding_present)
            {
                if (is_chunked)
                {
                    enum ChunkedState
                    {
                        ReadChunkSize,
                        ReadChunkData,
                        ReadChunkCRLFAfterData
                    };
                    ChunkedState state = ReadChunkSize;

                    uint32_t chunk_expected_data_size = 0;
                    // parse chunks
                    while (true)
                    {
                        bool action_taken = false;

                        if (input_buffer_end > input_buffer_start)
                        {
                            if (state == ReadChunkSize || state == ReadChunkCRLFAfterData)
                            {
                                const char *pattern = "\r\n";
                                // Only need to check 1 byte back for boundary case where '\r' is at read_check_start-1
                                // and '\n' is at read_check_start (pattern spans old/new data boundary)
                                uint32_t start_buffer_search = 0;
                                if (input_buffer_start >= 1)
                                    start_buffer_search = input_buffer_start - 1;
                                uint32_t crlf_pos = array_index_of(input_buffer, start_buffer_search, input_buffer_end, (const uint8_t *)pattern, 2);
                                bool found_crlf = crlf_pos < input_buffer_end;

                                if (found_crlf)
                                {
                                    action_taken = true;
                                    uint32_t chunk_size = 0;
                                    if (state == ReadChunkSize)
                                    {
                                        input_buffer[crlf_pos] = '\0';
                                        if (sscanf((const char *)input_buffer, "%x", &chunk_size) != 1)
                                        {
                                            printf("[HTTP] Invalid chunk size: %s\n", (const char *)input_buffer);
                                            return false;
                                        }
                                    }
                                    else
                                    {
                                        // crlf after data
                                        // crlf_pos needs to be 0
                                        if (crlf_pos != 0)
                                        {
                                            printf("[HTTP] Invalid chunk ending CRLF\n");
                                            return false;
                                        }
                                    }

                                    // move the remaining data to the beginning of the buffer
                                    uint32_t line_ending_pos = crlf_pos + 2;
                                    input_buffer_end -= line_ending_pos;
                                    input_buffer_start = 0;
                                    memcpy_custom(input_buffer, &input_buffer[line_ending_pos], input_buffer_end);

                                    if (state == ReadChunkSize)
                                    {
                                        // ending of chunks detected
                                        if (chunk_size == 0)
                                            return callbacks.onComplete();
                                        // read chunk data until reaches chunk_size
                                        chunk_expected_data_size = chunk_size;
                                        state = ReadChunkData;
                                    }
                                    else if (state == ReadChunkCRLFAfterData)
                                        state = ReadChunkSize;
                                    continue;
                                }
                                else
                                    input_buffer_start = input_buffer_end; // update read_check_start before reading more data
                            }
                            else if (state == ReadChunkData)
                            {
                                action_taken = true;

                                uint32_t range = input_buffer_end - input_buffer_start;
                                if (range > input_buffer_size)
                                {
                                    printf("[HTTP] Logic error, chunk data range exceeds input buffer size\n");
                                    return false;
                                }

                                if (range <= chunk_expected_data_size)
                                {
                                    chunk_expected_data_size -= range;
                                    // all data can be passed to the body part
                                    if (!callbacks.onBodyPart(&input_buffer[input_buffer_start], range))
                                        return false;
                                    input_buffer_start = 0;
                                    input_buffer_end = 0;
                                }
                                else
                                {
                                    // the readed bytes exceed the chunk expected size

                                    if (!callbacks.onBodyPart(&input_buffer[input_buffer_start], chunk_expected_data_size))
                                        return false;

                                    // move the remaining data to the beginning of the buffer
                                    uint32_t line_ending_pos = chunk_expected_data_size;
                                    input_buffer_end -= line_ending_pos;
                                    input_buffer_start = 0;
                                    memcpy_custom(input_buffer, &input_buffer[line_ending_pos], input_buffer_end);

                                    chunk_expected_data_size = 0;
                                }

                                if (chunk_expected_data_size == 0)
                                {
                                    // no more data to read, move to CRLF after data state
                                    state = ReadChunkCRLFAfterData;
                                    continue;
                                }
                            }
                        }

                        if (!action_taken)
                        {
                            // need more data
                            // read_check_start = readed_bytes_total;
                            if (input_buffer_end >= input_buffer_size)
                            {
                                printf("[HTTP] Logic error, cannot read more data into buffer\n");
                                return false;
                            }
                            uint32_t readed_feedback;
                            if (!socket->read_buffer(input_buffer + input_buffer_end, input_buffer_size - input_buffer_end, &readed_feedback))
                            {
                                if (socket->isReadTimedout())
                                    printf("[HTTP] Socket read timed out\n");
                                else
                                    printf("[HTTP] Connection or thread interrupted with the read feedback: %u (must be 0)\n", input_buffer_end);
                                return false;
                            }
                            input_buffer_end += readed_feedback;
                        }
                    }
                }
                else
                {
                    printf("[HTTP] Unsupported Transfer-Encoding\n");
                    return false;
                }
            }
            else if (content_length_present)
            {
                uint32_t range = input_buffer_end - input_buffer_start;
                if (range > input_buffer_size)
                {
                    printf("[HTTP] Logic error, chunk data range exceeds input buffer size\n");
                    return false;
                }

                uint32_t total_read = (std::min)(range, content_length);

                if (total_read > 0)
                {
                    // the remaining buffer has body data
                    if (!callbacks.onBodyPart(&input_buffer[input_buffer_start], total_read))
                        return false;
                    input_buffer_start = 0;
                    input_buffer_end = 0;
                }

                while (total_read < content_length)
                {
                    uint32_t to_read = (std::min)(content_length - total_read, input_buffer_size);
                    if (!socket->read_buffer(input_buffer, to_read, &input_buffer_end))
                    {
                        if (socket->isClosed())
                            printf("[HTTP] Connection closed before reading all body data\n");
                        else if (socket->isReadTimedout())
                            printf("[HTTP] Socket read timed out\n");
                        else
                            printf("[HTTP] Connection or thread interrupted with the read feedback: %u (must be 0)\n", input_buffer_end);
                        return false;
                    }

                    total_read += input_buffer_end;

                    if (!callbacks.onBodyPart(input_buffer, input_buffer_end))
                        return false;
                }

                return callbacks.onComplete();
            }
            else if (read_body_until_connection_close)
            {
                // read until connection close
                if (input_buffer_end > input_buffer_start)
                {
                    uint32_t range = input_buffer_end - input_buffer_start;
                    if (range > input_buffer_size)
                    {
                        printf("[HTTP] Logic error, chunk data range exceeds input buffer size\n");
                        return false;
                    }
                    // the remaining buffer has body data
                    if (!callbacks.onBodyPart(&input_buffer[input_buffer_start], range))
                        return false;
                    input_buffer_start = 0;
                    input_buffer_end = 0;
                }

                while (true)
                {
                    if (!socket->read_buffer(input_buffer, input_buffer_size, &input_buffer_end))
                    {
                        if (socket->isClosed())
                            break; // connection closed
                        else if (socket->isReadTimedout())
                            printf("[HTTP] Socket read timed out\n");
                        else
                            printf("[HTTP] Connection or thread interrupted with the read feedback: %u (must be 0)\n", input_buffer_end);
                        return false;
                    }

                    if (!callbacks.onBodyPart(input_buffer, input_buffer_end))
                        return false;
                }

                return callbacks.onComplete();
            }

            return true;
        }

        bool HTTPBase::readFromStream(Platform::SocketTCP *socket, bool read_body_until_connection_close)
        {
            clear();
            std::vector<uint8_t> input_buffer(HTTP_READ_BUFFER_CHUNK_SIZE);
            return parseHTTPStream(
                socket,
                {// onFirstLine
                 [this](const char *main_header)
                 {
                     return this->read_first_line(std::string(main_header));
                 },
                 // onHeader
                 [this](const char *key, const char *value)
                 {
                        this->headers[std::string(key)] = std::string(value);
                        return true; },
                 // onHeadersComplete
                 [](const uint8_t *remaining_data, uint32_t size)
                 {
                        // no special action needed
                        return true; },
                 // onBodyPart
                 [this](const uint8_t *data, uint32_t size)
                 {
                    if (this->body.size() + size > HTTP_MAX_BODY_SIZE)
                    {
                        printf("[HTTP] Body size exceeds maximum allowed size: %u bytes\n", HTTP_MAX_BODY_SIZE);
                        return false;
                    }
                    this->body.insert(this->body.end(), data, data + size);
                    return true; },
                 // onComplete
                 []()
                 { return true; }},
                input_buffer.data(),             // input buffer
                (uint32_t)input_buffer.size(),   // input buffer size
                HTTP_MAX_HEADER_RAW_SIZE,        // max_header_size_bytes
                HTTP_MAX_HEADER_COUNT,           // max header count
                read_body_until_connection_close // read body until connection close
            );
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

        const std::vector<uint8_t> &HTTPBase::bodyAsVector() const
        {
            return body;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif