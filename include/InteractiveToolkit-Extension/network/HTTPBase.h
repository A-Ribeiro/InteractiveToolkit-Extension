#pragma once

// #include "SocketTCP_SSL.h"
#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/EventCore/Callback.h>

#include "parser/HTTPParser.h"

namespace Platform
{
    class SocketTCP;
}

namespace ITKExtension
{
    namespace Network
    {

        // for receiving
        const int HTTP_READ_BUFFER_CHUNK_SIZE = 32 * 1024; // 32 KB -- the implementation of parseHTTPStream uses 2 buffers with this size
        // const int HTTP_MAX_HEADER_RAW_SIZE = HTTP_READ_BUFFER_CHUNK_SIZE - 2;
        const int HTTP_MAX_HEADER_COUNT = 100;

        // for transmitting
        const int HTTP_TRANSFER_ENCODING_MAX_SIZE = 16 * 1024; // 16 KB

        // HTTP HTTPRequest & HTTPResponse
        const int HTTP_MAX_BODY_SIZE = 100 * 1024 * 1024; // 100 MB

        class HTTPBase
        {
        protected:
            virtual bool read_first_line(const std::string &firstLine) = 0;
            virtual std::string mount_first_line() = 0;

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

            void eraseHeader(const std::string &key);

            // Case-insensitive header find (HTTP headers are case-insensitive per RFC 7230)
            std::unordered_map<std::string, std::string>::const_iterator findHeaderCaseInsensitive(const std::string &key) const;

            HTTPBase &setHeader(const std::string &key,
                                const std::string &value);

            HTTPBase &setBody(const std::string &body = "",
                              const std::string &content_type = "text/plain");

            HTTPBase &setBody(const uint8_t *body, uint32_t body_size,
                              const std::string &content_type = "application/octet-stream");

            std::string bodyAsString() const;
            const std::vector<uint8_t> &bodyAsVector() const;
        };

    }
}