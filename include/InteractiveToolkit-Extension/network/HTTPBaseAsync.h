#pragma once

#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/EventCore/Callback.h>
#include <unordered_map>
#include <vector>
#include <string>

#include "parser/HTTPParser.h"
#include "parser/HTTPWriter.h"

namespace ITKExtension
{
    namespace Network
    {
        // HTTP HTTPRequestAsync & HTTPResponseAsync
        const int HTTP_ASYNC_MAX_BODY_SIZE = 100 * 1024 * 1024; // 100 MB

        class BodyProvider
        {
        public:
            virtual ~BodyProvider() = default;

            // reset reading
            virtual void body_read_start() = 0;

            // Returns total size of the body, if -1 force Transfer-Encoding: chunked
            virtual int32_t body_read_get_size() = 0;

            // read binary data into buffer, returns number of bytes read
            // returns 0 when done
            virtual uint32_t body_read(uint8_t *buffer, uint32_t max_size) = 0;
        };

        class BodyConsumer : public BodyProvider
        {
        public:
            virtual ~BodyConsumer() = default;

            // start a new body
            virtual void body_write_start() = 0;

            // Append a chunk of data to the body
            virtual uint32_t body_write(const uint8_t *data, uint32_t size) = 0;
        };

        class HTTPBaseAsync : public EventCore::HandleCallback
        {
        protected:
            // virtual bool read_first_line(const std::string &firstLine) = 0;
            // virtual std::string mount_first_line() = 0;

            std::unordered_map<std::string, std::string> headers;

            std::shared_ptr<BodyProvider> bodyProvider;
            std::shared_ptr<BodyConsumer> bodyConsumer;

            void setBodyContentLength(uint32_t body_size, const std::string &content_type);

            // Case-insensitive header find (HTTP headers are case-insensitive per RFC 7230)
            std::unordered_map<std::string, std::string>::const_iterator findHeaderCaseInsensitive(const std::string &key) const;
            std::unordered_map<std::string, std::string>::iterator findHeaderCaseInsensitive(const std::string &key);

        public:
            HTTPBaseAsync() = default;
            virtual ~HTTPBaseAsync() = default;

            virtual void clear() = 0;

            const std::unordered_map<std::string, std::string> &listHeaders() const;

            // header ops
            bool hasHeader(const std::string &key) const;
            std::string getHeader(const std::string &key) const;
            void eraseHeader(const std::string &key);
            HTTPBaseAsync &setHeader(const std::string &key, const std::string &value);

            // body ops
            bool setBody(const std::string &body, const std::string &content_type = "text/plain");
            bool setBody(const uint8_t *body, uint32_t body_size,
                         const std::string &content_type = "application/octet-stream");

            // false on error
            bool appendToBody(const uint8_t *data, uint32_t size,
                              const std::string &content_type = "application/octet-stream");

            void setBodyProvider(std::shared_ptr<BodyProvider> provider,
                                 const std::string &content_type = "application/octet-stream");

            std::string bodyAsString() const;
            std::vector<uint8_t> bodyAsVector() const;
            void bodySetString(std::string *output) const;
            void bodySetVector(std::vector<uint8_t> *output) const;

            void setBodyConsumer(std::shared_ptr<BodyConsumer> consumer);

            std::shared_ptr<BodyProvider> getBodyProvider();
            std::shared_ptr<BodyConsumer> getBodyConsumer();

            std::shared_ptr<BodyProvider> defaultBodyProvider();
            std::shared_ptr<BodyConsumer> defaultBodyConsumer();

            virtual HTTPParserCallbacks getHTTPParserCallbacks() = 0;
            virtual HTTPWriterCallbacks getHTTPWriterCallbacks() = 0;

            virtual std::string firstLine() const = 0;
        };

    }
}
