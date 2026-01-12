#pragma once

#include "HTTPBaseAsync.h"

namespace ITKExtension
{
    namespace Network
    {

        class HTTPResponseAsync : public HTTPBaseAsync
        {
        protected:
            // bool read_first_line(const std::string &firstLine);
            // std::string mount_first_line();

            const std::string &getReasonPhraseFromStatusCode(int status_code) const;

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
            int status_code;
            std::string reason_phrase;
            std::string http_version;

            HTTPResponseAsync();

            void clear();

            HTTPResponseAsync &setDefault(
                int status_code = 200,
                std::string http_version = "HTTP/1.1");
            
            HTTPParserCallbacks getHTTPParserCallbacks();
            HTTPWriterCallbacks getHTTPWriterCallbacks();

            std::string firstLine() const;

            ITK_DECLARE_CREATE_SHARED(HTTPResponseAsync)
        };

    }
}
