#include <InteractiveToolkit-Extension/network/HTTPResponseAsync.h>
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

        const std::string &HTTPResponseAsync::getReasonPhraseFromStatusCode(int status_code) const
        {
            static const std::map<int, const std::string> status_reason_map = {
                {100, "Continue"},
                {101, "Switching Protocols"},
                {102, "Processing"},
                {200, "OK"},
                {201, "Created"},
                {202, "Accepted"},
                {203, "Non-Authoritative Information"},
                {204, "No Content"},
                {205, "Reset Content"},
                {206, "Partial Content"},
                {207, "Multi-Status"},
                {300, "Multiple Choices"},
                {301, "Moved Permanently"},
                {302, "Found"},
                {303, "See Other"},
                {304, "Not Modified"},
                {305, "Use Proxy"},
                {307, "Temporary Redirect"},
                {400, "Bad Request"},
                {401, "Unauthorized"},
                {402, "Payment Required"},
                {403, "Forbidden"},
                {404, "Not Found"},
                {405, "Method Not Allowed"},
                {406, "Not Acceptable"},
                {407, "Proxy Authentication Required"},
                {408, "Request Timeout"},
                {409, "Conflict"},
                {410, "Gone"},
                {411, "Length Required"},
                {412, "Precondition Failed"},
                {413, "Payload Too Large"},
                {414, "URI Too Long"},
                {415, "Unsupported Media Type"},
                {416, "Range Not Satisfiable"},
                {417, "Expectation Failed"},
                {426, "Upgrade Required"},
                {500, "Internal Server Error"},
                {501, "Not Implemented"},
                {502, "Bad Gateway"},
                {503, "Service Unavailable"},
                {504, "Gateway Timeout"},
                {505, "HTTP Version Not Supported"}};
            static const std::string unknown = "Unknown";
            auto it = status_reason_map.find(status_code);
            if (it != status_reason_map.end())
                return it->second;
            else
                return unknown;
        }

        HTTPResponseAsync::HTTPResponseAsync()
        {
            bodyProvider = defaultBodyProvider();
            bodyConsumer = defaultBodyConsumer();
        }

        void HTTPResponseAsync::clear()
        {
            headers.clear();
            status_code = 0;
            reason_phrase.clear();
            http_version.clear();

            bodyProvider->body_read_start();
            bodyConsumer->body_write_start();
        }

        HTTPResponseAsync &HTTPResponseAsync::setDefault(
            int status_code,
            std::string http_version)
        {
            headers.clear();
            headers["Connection"] = "keep-alive";

            this->status_code = status_code;
            this->reason_phrase = getReasonPhraseFromStatusCode(status_code);
            this->http_version = http_version;

            bodyProvider->body_read_start();
            bodyConsumer->body_write_start();

            return *this;
        }

        HTTPParserCallbacks HTTPResponseAsync::getHTTPParserCallbacks()
        {
            HTTPParserCallbacks callbacks;

            callbacks.onFirstLine = EventCore::CallbackWrapper(&HTTPResponseAsync::parser_onFirstLine, this);
            callbacks.onHeader = EventCore::CallbackWrapper(&HTTPResponseAsync::parser_onHeader, this);
            callbacks.onHeadersComplete = EventCore::CallbackWrapper(&HTTPResponseAsync::parser_onHeadersComplete, this);
            callbacks.onBodyPart = EventCore::CallbackWrapper(&HTTPResponseAsync::parser_onBodyPart, this);
            callbacks.onComplete = EventCore::CallbackWrapper(&HTTPResponseAsync::parser_onComplete, this);
            return callbacks;
        }

        HTTPWriterCallbacks HTTPResponseAsync::getHTTPWriterCallbacks()
        {
            HTTPWriterCallbacks callbacks;

            callbacks.getHeaderCount = EventCore::CallbackWrapper(&HTTPResponseAsync::writer_getHeaderCount, this);
            callbacks.getHeader = EventCore::CallbackWrapper(&HTTPResponseAsync::writer_getHeader, this);
            callbacks.startBodyStreaming = EventCore::CallbackWrapper(&HTTPResponseAsync::writer_startBodyStreaming, this);
            callbacks.getBodyPart = EventCore::CallbackWrapper(&HTTPResponseAsync::writer_getBodyPart, this);

            return callbacks;
        }

        std::string HTTPResponseAsync::firstLine() const 
        {
            return ITKCommon::PrintfToStdString("%s %d %s", http_version.c_str(), status_code, reason_phrase.c_str());
        }

        // parser callbacks
        bool HTTPResponseAsync::parser_onFirstLine(const char *first_line)
        {
            if (!is_valid_http_header_str(first_line))
            {
                printf("[HTTPRequestAsync] Invalid characters in HTTP first line: %s\n", first_line);
                return false;
            }
            // Parse the first line of the HTTP response: HTTP/1.1 200 OK
            auto parts = ITKCommon::StringUtil::tokenizer(first_line, " ");
            if (parts.size() >= 3 && ITKCommon::StringUtil::startsWith(parts[0], "HTTP/"))
            {
                http_version = parts[0];
                if (sscanf(parts[1].c_str(), "%i", &status_code) != 1)
                {
                    printf("[HTTPResponseAsync] Invalid status code format: %s\n", parts[1].c_str());
                    return false;
                }
                reason_phrase = getReasonPhraseFromStatusCode(status_code);
                return true;
            }
            return false;
        }
        bool HTTPResponseAsync::parser_onHeader(const char *key, const char *value)
        {
            if (!is_valid_http_header_str(key))
            {
                printf("[HTTPRequestAsync] Invalid characters in HTTP header key: %s\n", key);
                return false;
            }
            if (!is_valid_http_header_str(value))
            {
                printf("[HTTPRequestAsync] Invalid characters in HTTP header value: %s\n", value);
                return false;
            }
            setHeader(std::string(key), std::string(value));
            return true;
        }
        bool HTTPResponseAsync::parser_onHeadersComplete(const uint8_t *remaining_data, uint32_t size)
        {
            return true;
        }
        bool HTTPResponseAsync::parser_onBodyPart(const uint8_t *data, uint32_t size)
        {
            if (bodyConsumer->body_read_get_size() + size > HTTP_ASYNC_MAX_BODY_SIZE)
            {
                printf("[HTTPResponseAsync] Body size limit exceeded (%u bytes)\n", HTTP_ASYNC_MAX_BODY_SIZE);
                return false;
            }
            uint32_t offset = 0;
            while (offset < size)
            {
                uint32_t written = bodyConsumer->body_write(data + offset, size - offset);
                if (written == 0)
                    return false;
                offset += written;
            }
            return true;
        }
        bool HTTPResponseAsync::parser_onComplete()
        {
            return true;
        }

        // writer callbacks
        bool HTTPResponseAsync::writer_getHeader(int header_index, char *string_buffer_output, uint32_t string_buffer_max_bytes, uint32_t *str_length)
        {
            if (header_index == 0)
            {
                *str_length = snprintf(string_buffer_output, string_buffer_max_bytes, "%s %d %s\r\n", http_version.c_str(), status_code, reason_phrase.c_str());
                return true;
            }
            auto header_it = headers.cbegin();
            std::advance(header_it, header_index - 1);
            if (header_it != headers.cend())
            {
                const auto &key = header_it->first;
                const auto &value = header_it->second;
                *str_length = snprintf(string_buffer_output, string_buffer_max_bytes, "%s: %s\r\n", key.c_str(), value.c_str());
                return true;
            }
            return false;
        }
        int32_t HTTPResponseAsync::writer_getHeaderCount()
        {
            return (int32_t)headers.size() + 1;
        }
        bool HTTPResponseAsync::writer_startBodyStreaming(bool *is_body_chunked, int32_t *total_size)
        {
            *is_body_chunked = getHeader("Transfer-Encoding").find("chunked") != std::string::npos;
            *total_size = bodyProvider->body_read_get_size();
            if ((*total_size) < 0 && !(*is_body_chunked))
            {
                printf("[HTTPRequestAsync] Body size unknown and Transfer-Encoding is not chunked\n");
                return false;
            }
            bodyProvider->body_read_start();
            return true;
        }
        bool HTTPResponseAsync::writer_getBodyPart(uint8_t *data_output, uint32_t buffer_size, uint32_t *written)
        {
            *written = bodyProvider->body_read(data_output, buffer_size);
            return true;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif
