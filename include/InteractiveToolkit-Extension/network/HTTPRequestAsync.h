#pragma once

#include "HTTPBaseAsync.h"

namespace ITKExtension
{
    namespace Network
    {

        class HTTPRequestAsync : public HTTPBaseAsync
        {
        protected:
            // bool read_first_line(const std::string &firstLine);
            // std::string mount_first_line();

            // parser callbacks
            bool parser_onFirstLine(const char *main_header);
            bool parser_onHeader(const char *key, const char *value);
            bool parser_onHeadersComplete(const uint8_t *remaining_data, uint32_t size);
            bool parser_onBodyPart(const uint8_t *data, uint32_t size);
            bool parser_onComplete();
            // writer callbacks
            bool writer_getHeader(int header_index, char *string_buffer_output, uint32_t string_buffer_max_bytes, uint32_t *str_length);
            int32_t writer_getHeaderCount();
            bool writer_startBodyStreaming(bool *is_body_chunked, int32_t *total_size);
            bool writer_getBodyPart(uint8_t *data_output, uint32_t buffer_size, uint32_t *written);


        public:
            std::string method;
            std::string path;
            std::string http_version;

            HTTPRequestAsync();

            void clear();

            HTTPRequestAsync &setDefault(
                std::string host = "subdomain.example.com",
                std::string method = "GET",
                std::string path = "/",
                std::string http_version = "HTTP/1.1");

            HTTPParserCallbacks getHTTPParserCallbacks();
            HTTPWriterCallbacks getHTTPWriterCallbacks();

            std::string firstLine() const;

            ITK_DECLARE_CREATE_SHARED(HTTPRequestAsync)
        };

    }
}
