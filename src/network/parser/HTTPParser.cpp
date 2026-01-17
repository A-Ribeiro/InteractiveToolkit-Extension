#include <InteractiveToolkit-Extension/network/parser/HTTPParser.h>
#include <InteractiveToolkit/AlgorithmCore/PatternMatch/array_index_of.h>

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
            return memcpy(dst, src, size);
            // // non overlapping buffers
            // if (src > dst + size || dst > src + size)
            //     return memcpy(dst, src, size);
            // // overlapping buffers, copy byte by byte
            // for (uint32_t i = 0; i < size; i++)
            //     dst[i] = src[i];
            // return dst;
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

        HTTPParser::HTTPParser(uint32_t input_buffer_size,
                               uint32_t max_header_count)
        {
            this->input_buffer_size = input_buffer_size;
            this->max_header_size_bytes = input_buffer_size - 2; // needs to accomodate CRLF
            this->max_header_count = max_header_count;
            reading_buffer = STL_Tools::make_unique<uint8_t[]>(input_buffer_size * 2);
            this->input_buffer_a = reading_buffer.get();
            this->input_buffer_b = reading_buffer.get() + this->input_buffer_size;
        }
        HTTPParser::~HTTPParser()
        {
            connectionClosed();
        }

        void HTTPParser::initialize(bool bytes_after_headers_are_body_data,
                                    const HTTPParserCallbacks &callbacks)
        {
            this->callbacks = callbacks;
            this->bytes_after_headers_are_body_data = bytes_after_headers_are_body_data;

            state = HTTPParserState::ReadingFirstLine;

            // reading headers
            input_buffer_start = 0;
            input_buffer_end = 0;
            // first_line = true;

            // body special headers
            content_length = 0;
            is_chunked = false;

            content_length_present = false;
            transfer_encoding_present = false;
        }

        HTTPParserState HTTPParser::insertData(const uint8_t *data, uint32_t size, uint32_t *inserted_bytes)
        {
            *inserted_bytes = 0;
            uint32_t offset = 0;
            while (offset < size)
            {
                uint32_t available_space = input_buffer_size - input_buffer_end;
                uint32_t to_insert = (std::min)(size - offset, available_space);

                uint32_t inserted;
                HTTPParserState res = _insertData(data + offset, to_insert, &inserted);
                if (res == HTTPParserState::Complete ||
                    res == HTTPParserState::Error)
                    return state;
                else if (res == HTTPParserState::ReadingHeadersReady)
                {
                    *inserted_bytes += inserted;
                    return state;
                }

                *inserted_bytes += inserted;
                offset += inserted;
            }
            return state;
        }
        HTTPParserState HTTPParser::_insertData(const uint8_t *data, uint32_t __size, uint32_t *inserted_bytes)
        {
            using namespace AlgorithmCore::PatternMatch;
            *inserted_bytes = 0;
            if (state == HTTPParserState::Complete || state == HTTPParserState::Error || __size == 0)
                return state;

            // reading data to input_buffer_a
            // available memory = input_buffer_size - input_buffer_end
            if (__size > input_buffer_size - input_buffer_end)
            {
                printf("[HTTP] overflow, increase the input_buffer_size.\n");
                state = HTTPParserState::Error;
                return state;
            }
            memcpy_custom(input_buffer_a + input_buffer_end, data, __size);
            int64_t insert_range_begin = (int64_t)input_buffer_end;
            input_buffer_end += __size;
            *inserted_bytes = __size;

            // apply after headers logic
            if (state == HTTPParserState::ReadingHeadersReady)
                return state;
            else if (state == HTTPParserState::ReadingHeadersReadyApplyNextBodyState)
            {
                state = body_state_after_headers;
                if (state == HTTPParserState::Complete)
                {
                    if (!callbacks.onComplete())
                    {
                        state = HTTPParserState::Error;
                        return state;
                    }
                    return state;
                }
            }

            if (state == HTTPParserState::ReadingFirstLine || state == HTTPParserState::ReadingHeaders)
            {
                while (true)
                {
                    const char *pattern = "\r\n";
                    // Only need to check 1 byte back for boundary case where '\r' is at read_check_start-1
                    // and '\n' is at read_check_start (pattern spans old/new data boundary)
                    uint32_t start_buffer_search = 0;
                    if (input_buffer_start >= 1)
                        start_buffer_search = input_buffer_start - 1;
                    uint32_t crlf_pos = array_index_of(input_buffer_a, start_buffer_search, input_buffer_end, (const uint8_t *)pattern, 2);
                    bool found_crlf = crlf_pos < input_buffer_end;

                    if (found_crlf)
                    {
                        if (crlf_pos > max_header_size_bytes)
                        {
                            printf("[HTTP] HTTP header too large: %u bytes\n", (uint32_t)(crlf_pos + 2));
                            state = HTTPParserState::Error;
                            return state;
                        }

                        input_buffer_a[crlf_pos] = '\0';
                        // check header bytes
                        if (!is_valid_http_header_str((const char *)input_buffer_a))
                        {
                            printf("[HTTP] Invalid character in HTTP header: %s\n", (const char *)input_buffer_a);
                            state = HTTPParserState::Error;
                            return state;
                        }

                        if (state == HTTPParserState::ReadingFirstLine)
                        {
                            if (crlf_pos == 0 || !callbacks.onFirstLine((const char *)input_buffer_a))
                            {
                                state = HTTPParserState::Error;
                                return state;
                            }
                            state = HTTPParserState::ReadingHeaders;
                        }
                        else if (crlf_pos > 0)
                        {
                            // normal header
                            const char *pattern_header = ": ";
                            size_t value_pos = array_index_of(input_buffer_a, 0, crlf_pos, (const uint8_t *)pattern_header, 2);
                            bool found_key_value = value_pos < crlf_pos;
                            if (!found_key_value)
                            {
                                printf("[HTTP] Invalid HTTP header format: %s\n", (const char *)input_buffer_a);
                                state = HTTPParserState::Error;
                                return state;
                            }

                            input_buffer_a[value_pos] = '\0';
                            const char *key = (const char *)input_buffer_a;
                            const char *value = (const char *)&input_buffer_a[value_pos + 2];

                            if (!content_length_present)
                                content_length_present |= checkContentLengthHeader(key, value, &content_length);
                            if (!transfer_encoding_present)
                                transfer_encoding_present |= checkTransferEncodingHeader(key, value, &is_chunked);

                            if (!callbacks.onHeader(key, value))
                            {
                                state = HTTPParserState::Error;
                                return state;
                            }

                            max_header_count--;
                            if (max_header_count == 0)
                            {
                                printf("[HTTP] Too many HTTP header lines readed...\n");
                                state = HTTPParserState::Error;
                                return state;
                            }
                        }

                        // move the remaining data to the beginning of the buffer
                        uint32_t line_ending_pos = crlf_pos + 2;
                        input_buffer_end -= line_ending_pos;
                        input_buffer_start = 0;
                        memcpy_custom(input_buffer_b, &input_buffer_a[line_ending_pos], input_buffer_end);
                        std::swap(input_buffer_a, input_buffer_b);

                        // check inserted bytes logic
                        insert_range_begin -= (int64_t)line_ending_pos;

                        if (crlf_pos == 0)
                        {
                            // headers ending detection
                            if (!callbacks.onHeadersComplete(input_buffer_a, input_buffer_end))
                            {
                                state = HTTPParserState::Error;
                                return state;
                            }

                            // update inserted bytes logic
                            //   if insert range begin is negative, part of the inserted data was consumed
                            //   otherwise, none was consumed
                            *inserted_bytes = (insert_range_begin >= 0) ? 0 : -insert_range_begin;

                            // force reinsert body for separate the parse from headers
                            input_buffer_start = 0;
                            input_buffer_end = 0;
                            total_read = 0;

                            // end of headers
                            // check for the body reading mode
                            if (transfer_encoding_present)
                            {
                                if (!is_chunked)
                                {
                                    printf("[HTTP] Unsupported Transfer-Encoding\n");
                                    state = HTTPParserState::Error;
                                    return state;
                                }
                                else
                                {
                                    body_state_after_headers = HTTPParserState::ReadingBodyChunked;
                                    state = HTTPParserState::ReadingHeadersReady;
                                    chunk_state = ReadChunkSize;
                                    chunk_expected_data_size = 0;

                                    // *inserted_bytes will return only the data readed from the headers,
                                    // and forcing the outside loop to reinsert the body data in next state
                                    return state;
                                }
                            }
                            else if (content_length_present)
                            {
                                body_state_after_headers = (content_length == 0) ? HTTPParserState::Complete : HTTPParserState::ReadingBodyContentLength;
                                state = HTTPParserState::ReadingHeadersReady;

                                // *inserted_bytes will return only the data readed from the headers,
                                // and forcing the outside loop to reinsert the body data in next state
                                return state;
                            }
                            else if (bytes_after_headers_are_body_data)
                            {
                                body_state_after_headers = HTTPParserState::ReadingBodyUntilConnectionClose;
                                state = HTTPParserState::ReadingHeadersReady;

                                // *inserted_bytes will return only the data readed from the headers,
                                // and forcing the outside loop to reinsert the body data in next state
                                return state;
                            }
                            else
                            {
                                body_state_after_headers = HTTPParserState::Complete;
                                state = HTTPParserState::ReadingHeadersReady;
                                return state;
                            }
                        }
                    }
                    else
                    {
                        // need more data
                        input_buffer_start = input_buffer_end;
                        return state;
                    }
                }
            }
            else if (state == HTTPParserState::ReadingBodyChunked)
            {
                while (true)
                {
                    // need more data...
                    if (input_buffer_start == input_buffer_end)
                        return state;

                    if (chunk_state == ReadChunkSize || chunk_state == ReadChunkCRLFAfterData)
                    {
                        const char *pattern = "\r\n";
                        // Only need to check 1 byte back for boundary case where '\r' is at read_check_start-1
                        // and '\n' is at read_check_start (pattern spans old/new data boundary)
                        uint32_t start_buffer_search = 0;
                        if (input_buffer_start >= 1)
                            start_buffer_search = input_buffer_start - 1;
                        uint32_t crlf_pos = array_index_of(input_buffer_a, start_buffer_search, input_buffer_end, (const uint8_t *)pattern, 2);
                        bool found_crlf = crlf_pos < input_buffer_end;

                        if (found_crlf)
                        {
                            uint32_t chunk_size = 0;
                            if (chunk_state == ReadChunkSize)
                            {
                                input_buffer_a[crlf_pos] = '\0';
                                if (sscanf((const char *)input_buffer_a, "%x", &chunk_size) != 1)
                                {
                                    printf("[HTTP] Invalid chunk size: %s\n", (const char *)input_buffer_a);
                                    state = HTTPParserState::Error;
                                    return state;
                                }
                            }
                            else
                            {
                                // crlf after data
                                // crlf_pos needs to be 0
                                if (crlf_pos != 0)
                                {
                                    printf("[HTTP] Invalid chunk ending CRLF\n");
                                    state = HTTPParserState::Error;
                                    return state;
                                }
                            }

                            // move the remaining data to the beginning of the buffer
                            uint32_t line_ending_pos = crlf_pos + 2;
                            input_buffer_end -= line_ending_pos;
                            input_buffer_start = 0;
                            memcpy_custom(input_buffer_b, &input_buffer_a[line_ending_pos], input_buffer_end);
                            std::swap(input_buffer_a, input_buffer_b);

                            if (chunk_state == ReadChunkSize)
                            {
                                // ending of chunks detected
                                if (chunk_size == 0)
                                {
                                    if (!callbacks.onComplete())
                                    {
                                        state = HTTPParserState::Error;
                                        return state;
                                    }
                                    state = HTTPParserState::Complete;
                                    return state;
                                }

                                // read chunk data until reaches chunk_size
                                chunk_expected_data_size = chunk_size;
                                chunk_state = ReadChunkData;
                            }
                            else if (chunk_state == ReadChunkCRLFAfterData)
                                chunk_state = ReadChunkSize;
                            continue;
                        }
                        else
                        {
                            // need more data
                            input_buffer_start = input_buffer_end;
                            return state;
                        }
                    }
                    else if (chunk_state == ReadChunkData)
                    {
                        uint32_t range = input_buffer_end - input_buffer_start;
                        if (range > input_buffer_size)
                        {
                            printf("[HTTP] Logic error, chunk data range exceeds input buffer size\n");
                            state = HTTPParserState::Error;
                            return state;
                        }

                        if (range <= chunk_expected_data_size)
                        {
                            chunk_expected_data_size -= range;
                            // all data can be passed to the body part
                            if (!callbacks.onBodyPart(&input_buffer_a[input_buffer_start], range))
                            {
                                state = HTTPParserState::Error;
                                return state;
                            }
                            input_buffer_start = 0;
                            input_buffer_end = 0;
                        }
                        else
                        {
                            // the readed bytes exceed the chunk expected size
                            if (!callbacks.onBodyPart(&input_buffer_a[input_buffer_start], chunk_expected_data_size))
                            {
                                state = HTTPParserState::Error;
                                return state;
                            }

                            // move the remaining data to the beginning of the buffer
                            uint32_t line_ending_pos = chunk_expected_data_size;
                            input_buffer_end -= line_ending_pos;
                            input_buffer_start = 0;
                            memcpy_custom(input_buffer_b, &input_buffer_a[line_ending_pos], input_buffer_end);
                            std::swap(input_buffer_a, input_buffer_b);

                            chunk_expected_data_size = 0;
                        }

                        if (chunk_expected_data_size == 0)
                        {
                            // no more data to read, move to CRLF after data chunk_state
                            chunk_state = ReadChunkCRLFAfterData;
                            continue;
                        }
                    }
                }
            }
            else if (state == HTTPParserState::ReadingBodyContentLength)
            {

                // check is complete
                if (total_read == content_length)
                {
                    if (!callbacks.onComplete())
                    {
                        state = HTTPParserState::Error;
                        return state;
                    }
                    state = HTTPParserState::Complete;
                    return state;
                }

                uint32_t remaining_to_read = content_length - total_read;
                input_buffer_end = (std::min)(input_buffer_end, remaining_to_read);

                if (input_buffer_end == 0)
                {
                    printf("[HTTP] Logic error, no data to read but still in ReadingBodyContentLength state\n");
                    state = HTTPParserState::Error;
                    return state;
                }

                total_read += input_buffer_end;

                if (!callbacks.onBodyPart(input_buffer_a, input_buffer_end))
                {
                    state = HTTPParserState::Error;
                    return state;
                }
                input_buffer_start = 0;
                input_buffer_end = 0;

                // check is complete
                if (total_read == content_length)
                {
                    if (!callbacks.onComplete())
                    {
                        state = HTTPParserState::Error;
                        return state;
                    }
                    state = HTTPParserState::Complete;
                    return state;
                }

                return state;
            }
            else if (state == HTTPParserState::ReadingBodyUntilConnectionClose)
            {
                if (!callbacks.onBodyPart(input_buffer_a, input_buffer_end))
                {
                    state = HTTPParserState::Error;
                    return state;
                }
                input_buffer_start = 0;
                input_buffer_end = 0;
                return state;
            }

            state = HTTPParserState::Error;
            return state;
        }

        // mandatory only when bytes_after_headers_are_body_data is true
        void HTTPParser::connectionClosed()
        {
            if (state == HTTPParserState::ReadingBodyUntilConnectionClose)
            {
                state = HTTPParserState::Complete;
                if (!callbacks.onComplete())
                    state = HTTPParserState::Error;
            }
            else if (state != HTTPParserState::Complete)
            {
                state = HTTPParserState::Error;
                printf("[HTTP] Connection closed before completing the HTTP parsing.\n");
            }
        }

        void HTTPParser::headersReadyApplyNextBodyState()
        {
            if (state == HTTPParserState::ReadingHeadersReady)
            {
                state = HTTPParserState::ReadingHeadersReadyApplyNextBodyState;
            }
            else
            {
                printf("[HTTP] Invalid operation: headersReadyApplyNextBodyState called when not in ReadingHeadersReady state\n");
                state = HTTPParserState::Error;
            }
        }

        void HTTPParser::checkOnlyStateTransition()
        {
            if (state == HTTPParserState::ReadingHeadersReadyApplyNextBodyState)
            {
                state = body_state_after_headers;
                if (state == HTTPParserState::ReadingBodyContentLength)
                {
                    if (content_length == 0)
                        state = HTTPParserState::Complete;
                }
                if (state == HTTPParserState::Complete)
                {
                    if (!callbacks.onComplete())
                        state = HTTPParserState::Error;
                }
            }
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif