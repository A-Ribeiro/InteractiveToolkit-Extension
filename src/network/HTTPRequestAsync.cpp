#include <InteractiveToolkit-Extension/network/HTTPRequestAsync.h>
#include <InteractiveToolkit/ITKCommon/StringUtil.h>

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

        HTTPRequestAsync::HTTPRequestAsync()
        {
            bodyProvider = defaultBodyProvider();
            bodyConsumer = defaultBodyConsumer();
        }

        void HTTPRequestAsync::clear()
        {
            headers.clear();

            method.clear();
            path.clear();
            http_version.clear();

            bodyProvider->body_read_start();
            bodyConsumer->body_write_start();
        }

        HTTPRequestAsync &HTTPRequestAsync::setDefault(
            std::string host,
            std::string method,
            std::string path,
            std::string http_version)
        {
            headers.clear();
            headers["Host"] = host;
            headers["Connection"] = "keep-alive";
            headers["User-Agent"] = "ITK-HTTP-Client/1.0";

            this->method = method;
            this->path = path;
            this->http_version = http_version;

            bodyProvider->body_read_start();
            bodyConsumer->body_write_start();

            return *this;
        }

        HTTPParserCallbacks HTTPRequestAsync::getHTTPParserCallbacks()
        {
            HTTPParserCallbacks callbacks;

            callbacks.onFirstLine = EventCore::CallbackWrapper(&HTTPRequestAsync::parser_onFirstLine, this);
            callbacks.onHeader = EventCore::CallbackWrapper(&HTTPRequestAsync::parser_onHeader, this);
            callbacks.onHeadersComplete = EventCore::CallbackWrapper(&HTTPRequestAsync::parser_onHeadersComplete, this);
            callbacks.onBodyPart = EventCore::CallbackWrapper(&HTTPRequestAsync::parser_onBodyPart, this);
            callbacks.onComplete = EventCore::CallbackWrapper(&HTTPRequestAsync::parser_onComplete, this);

            return callbacks;
        }

        HTTPWriterCallbacks HTTPRequestAsync::getHTTPWriterCallbacks()
        {
            HTTPWriterCallbacks callbacks;

            callbacks.getHeaderCount = EventCore::CallbackWrapper(&HTTPRequestAsync::writer_getHeaderCount, this);
            callbacks.getHeader = EventCore::CallbackWrapper(&HTTPRequestAsync::writer_getHeader, this);
            callbacks.startBodyStreaming = EventCore::CallbackWrapper(&HTTPRequestAsync::writer_startBodyStreaming, this);
            callbacks.getBodyPart = EventCore::CallbackWrapper(&HTTPRequestAsync::writer_getBodyPart, this);

            return callbacks;
        }

        std::string HTTPRequestAsync::firstLine() const 
        {
            return ITKCommon::PrintfToStdString("%s %s %s", method.c_str(), path.c_str(), http_version.c_str());
        }

        // parser callbacks
        bool HTTPRequestAsync::parser_onFirstLine(const char *first_line)
        {
            if (!is_valid_http_header_str(first_line))
            {
                printf("[HTTPRequestAsync] Invalid characters in HTTP first line: %s\n", first_line);
                return false;
            }
            auto parts = ITKCommon::StringUtil::tokenizer(first_line, " ");
            if (parts.size() == 3 && ITKCommon::StringUtil::startsWith(parts[2], "HTTP/"))
            {
                method = parts[0];
                path = parts[1];
                http_version = parts[2];
                if (method != "GET" &&
                    method != "POST" &&
                    method != "PUT" &&
                    method != "DELETE" &&
                    method != "HEAD" &&
                    method != "OPTIONS" &&
                    method != "PATCH" &&
                    method != "TRACE" &&
                    method != "CONNECT")
                {
                    printf("[HTTPRequestAsync] Unsupported HTTP Method: %s\n", method.c_str());
                    return false;
                }
                return true;
            }
            return false;
        }
        bool HTTPRequestAsync::parser_onHeader(const char *key, const char *value)
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
        bool HTTPRequestAsync::parser_onHeadersComplete(const uint8_t *remaining_data, uint32_t size)
        {
            return true;
        }
        bool HTTPRequestAsync::parser_onBodyPart(const uint8_t *data, uint32_t size)
        {
            if (bodyConsumer->body_read_get_size() + size > HTTP_ASYNC_MAX_BODY_SIZE)
            {
                printf("[HTTPRequestAsync] Body size limit exceeded (%u bytes)\n", HTTP_ASYNC_MAX_BODY_SIZE);
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
        bool HTTPRequestAsync::parser_onComplete()
        {
            return true;
        }

        // writer callbacks
        bool HTTPRequestAsync::writer_getHeader(int header_index, char *string_buffer_output, uint32_t string_buffer_max_bytes, uint32_t *str_length)
        {
            if (header_index == 0)
            {
                *str_length = snprintf(string_buffer_output, string_buffer_max_bytes, "%s %s %s\r\n", method.c_str(), path.c_str(), http_version.c_str());
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
        int32_t HTTPRequestAsync::writer_getHeaderCount()
        {
            return (int32_t)headers.size() + 1;
        }
        bool HTTPRequestAsync::writer_startBodyStreaming(bool *is_body_chunked, int32_t *total_size)
        {
            *is_body_chunked = getHeader("Transfer-Encoding").find("chunked") != std::string::npos;
            *total_size = bodyProvider->body_read_get_size();
            if ((*total_size) < 0 && !(*is_body_chunked))
            {
                printf("[HTTPRequestAsync] Body size unknown and Transfer-Encoding is not chunked\n");
                return false;
            }
            return true;
        }
        bool HTTPRequestAsync::writer_getBodyPart(uint8_t *data_output, uint32_t buffer_size, uint32_t *written)
        {
            *written = bodyProvider->body_read(data_output, buffer_size);
            return true;
        }
    }
}
