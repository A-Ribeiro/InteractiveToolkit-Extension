#pragma once

// #include "SocketTCP_SSL.h"
#include <InteractiveToolkit/common.h>

namespace Platform
{
    class SocketTCP;
}

namespace ITKExtension
{
    namespace Network
    {

        const int HTTP_MAX_HEADER_COUNT = 100;
        const int HTTP_MAX_HEADER_RAW_SIZE = 1024;
        const int HTTP_READ_BUFFER_CHUNK_SIZE = 4 * 1024; // 4 KB
        const int HTTP_MAX_BODY_SIZE = 100 * 1024 * 1024; // 100 MB

        const int HTTP_TRANSFER_ENCODING_MAX_SIZE = 16 * 1024; // 16 KB

        class HTTPBase
        {
        protected:
            virtual bool read_first_line(const std::string &firstLine) = 0;
            virtual std::string mount_first_line() = 0;

            static bool is_valid_header_character(char _chr);

            std::unordered_map<std::string, std::string> headers;
            std::vector<uint8_t> body;

        public:
            HTTPBase() = default;
            virtual ~HTTPBase() = default;

            virtual void clear() = 0;

            bool readFromStream(Platform::SocketTCP *socket, bool read_body_until_connection_close);

            bool writeToStream(Platform::SocketTCP *socket);

            const std::unordered_map<std::string, std::string> &listHeaders() const;

            bool hasHeader(const std::string &key) const;

            std::string getHeader(const std::string &key) const;

            // Case-insensitive header find (HTTP headers are case-insensitive per RFC 7230)
            std::unordered_map<std::string, std::string>::const_iterator findHeaderCaseInsensitive(const std::string &key) const;

            HTTPBase &setHeader(const std::string &key,
                                const std::string &value);

            HTTPBase &setBody(const std::string &body = "",
                              const std::string &content_type = "text/plain");

            HTTPBase &setBody(const uint8_t *body, uint32_t body_size,
                              const std::string &content_type = "application/octet-stream");

            std::string bodyAsString() const;
        };

    }
}