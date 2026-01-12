#include <InteractiveToolkit-Extension/network/HTTPBaseAsync.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {
        static inline int strcasecmp_custom(const char *s1, const char *s2)
        {
#if defined(_WIN32)
            return _stricmp(s1, s2);
#else
            return strcasecmp(s1, s2);
#endif
        }

        std::unordered_map<std::string, std::string>::const_iterator HTTPBaseAsync::findHeaderCaseInsensitive(const std::string &key) const
        {
            for (auto it = headers.begin(); it != headers.end(); ++it)
            {
                if (it->first.size() == key.size() && strcasecmp_custom(it->first.c_str(), key.c_str()) == 0)
                    return it;
            }
            return headers.end();
        }

        std::unordered_map<std::string, std::string>::iterator HTTPBaseAsync::findHeaderCaseInsensitive(const std::string &key)
        {
            for (auto it = headers.begin(); it != headers.end(); ++it)
            {
                if (it->first.size() == key.size() && strcasecmp_custom(it->first.c_str(), key.c_str()) == 0)
                    return it;
            }
            return headers.end();
        }

        const std::unordered_map<std::string, std::string> &HTTPBaseAsync::listHeaders() const
        {
            return headers;
        }

        bool HTTPBaseAsync::hasHeader(const std::string &key) const
        {
            return findHeaderCaseInsensitive(key) != headers.end();
        }

        std::string HTTPBaseAsync::getHeader(const std::string &key) const
        {
            auto it = findHeaderCaseInsensitive(key);
            if (it != headers.end())
                return it->second;
            return "";
        }

        void HTTPBaseAsync::eraseHeader(const std::string &key)
        {
            auto it = findHeaderCaseInsensitive(key);
            if (it != headers.end())
                headers.erase(it);
        }

        HTTPBaseAsync &HTTPBaseAsync::setHeader(const std::string &key,
                                                const std::string &value)
        {
            if (value.length() == 0)
            { // erase header if value is empty
                eraseHeader(key);
                return *this;
            }
            auto it = findHeaderCaseInsensitive(key);
            if (it != headers.end())
            {
                it->second = value;
                return *this;
            }
            headers[key] = value;
            return *this;
        }

        bool HTTPBaseAsync::setBody(const std::string &body,
                                    const std::string &content_type)
        {
            uint32_t size = (uint32_t)body.length();
            uint32_t offset = 0;
            bodyConsumer->body_write_start();
            while (offset < size)
            {
                uint32_t written = bodyConsumer->body_write((const uint8_t *)body.data() + offset, size - offset);
                if (written == 0)
                    return false;
                offset += written;
            }
            setBodyContentLength(size, content_type);
            return true;
        }

        bool HTTPBaseAsync::setBody(const uint8_t *body, uint32_t body_size,
                                    const std::string &content_type)
        {
            uint32_t size = (uint32_t)body_size;
            bodyConsumer->body_write_start();
            uint32_t offset = 0;
            while (offset < size)
            {
                uint32_t written = bodyConsumer->body_write(body + offset, size - offset);
                if (written == 0)
                    return false;
                offset += written;
            }
            setBodyContentLength(size, content_type);
            return true;
        }

        std::string HTTPBaseAsync::bodyAsString() const
        {
            uint32_t total_size = bodyConsumer->body_read_get_size();
            std::string body(total_size, '\0');
            bodySetString(&body);
            return body;
        }

        std::vector<uint8_t> HTTPBaseAsync::bodyAsVector() const
        {
            uint32_t total_size = bodyConsumer->body_read_get_size();
            std::vector<uint8_t> body(total_size);
            bodySetVector(&body);
            return body;
        }

        void HTTPBaseAsync::bodySetString(std::string *output) const
        {
            bodyConsumer->body_read_start();
            int32_t total_size = bodyConsumer->body_read_get_size();
            if (total_size < 0)
            {
                // unknown total size
                output->clear();
                const uint32_t chunk_size = 4096;
                std::vector<uint8_t> buffer(chunk_size);
                while (true)
                {
                    uint32_t read = bodyConsumer->body_read(buffer.data(), chunk_size);
                    if (read == 0)
                        break;
                    output->append((const char *)buffer.data(), read);
                }
                return;
            }
            // known total size
            output->resize(total_size);
            uint32_t read_offset = 0;
            while (true)
            {
                uint32_t read = bodyConsumer->body_read((uint8_t *)output->data() + read_offset, total_size - read_offset);
                if (read == 0)
                    break;
                read_offset += read;
            }
        }

        void HTTPBaseAsync::bodySetVector(std::vector<uint8_t> *output) const
        {
            bodyConsumer->body_read_start();
            int32_t total_size = bodyConsumer->body_read_get_size();
            if (total_size < 0)
            {
                // unknown total size
                output->clear();
                const uint32_t chunk_size = 4096;
                std::vector<uint8_t> buffer(chunk_size);
                while (true)
                {
                    uint32_t read = bodyConsumer->body_read(buffer.data(), chunk_size);
                    if (read == 0)
                        break;
                    output->insert(output->end(), buffer.data(), buffer.data() + read);
                }
                return;
            }
            // known total size
            output->resize(total_size);
            uint32_t read_offset = 0;
            while (true)
            {
                uint32_t read = bodyConsumer->body_read(output->data() + read_offset, total_size - read_offset);
                if (read == 0)
                    break;
                read_offset += read;
            }
        }

        bool HTTPBaseAsync::appendToBody(const uint8_t *data, uint32_t append_size, const std::string &content_type)
        {
            uint32_t size = (uint32_t)append_size;
            uint32_t offset = 0;
            while (offset < size)
            {
                uint32_t written = bodyConsumer->body_write(data + offset, size - offset);
                if (written == 0)
                    return false;
                offset += written;
            }
            setBodyContentLength(bodyConsumer->body_read_get_size(), content_type);
            return true;
        }

        void HTTPBaseAsync::setBodyProvider(std::shared_ptr<BodyProvider> provider,
                                            const std::string &content_type)
        {
            bodyProvider = provider;
            int32_t size = bodyProvider->body_read_get_size();
            if (size < 0)
            {
                // Force chunked encoding
                eraseHeader("Content-Length");
                setHeader("Transfer-Encoding", "chunked");
                return;
            }
            setBodyContentLength((uint32_t)size, content_type);
        }

        void HTTPBaseAsync::setBodyContentLength(uint32_t body_size, const std::string &content_type)
        {
            if (body_size > 0)
            {
                setHeader("Content-Type", content_type);
                setHeader("Content-Length", std::to_string(body_size));
            }
            else
            {
                eraseHeader("Content-Type");
                eraseHeader("Content-Length");
            }
        }

        std::shared_ptr<BodyProvider> HTTPBaseAsync::defaultBodyProvider()
        {
            class DefaultBodyProvider : public BodyProvider
            {
            public:
                HTTPBaseAsync *owner;
                DefaultBodyProvider(HTTPBaseAsync *_owner) : owner(_owner) {}
                void body_read_start() override
                {
                    owner->getBodyConsumer()->body_read_start();
                }

                int32_t body_read_get_size() override
                {
                    return owner->getBodyConsumer()->body_read_get_size();
                }

                uint32_t body_read(uint8_t *buffer, uint32_t max_size) override
                {
                    return owner->getBodyConsumer()->body_read(buffer, max_size);
                }
            };
            return std::make_shared<DefaultBodyProvider>(this);
        }

        void HTTPBaseAsync::setBodyConsumer(std::shared_ptr<BodyConsumer> consumer)
        {
            bodyConsumer = consumer;
        }

        std::shared_ptr<BodyProvider> HTTPBaseAsync::getBodyProvider()
        {
            return bodyProvider;
        }
        std::shared_ptr<BodyConsumer> HTTPBaseAsync::getBodyConsumer()
        {
            return bodyConsumer;
        }

        std::shared_ptr<BodyConsumer> HTTPBaseAsync::defaultBodyConsumer()
        {
            class DefaultBodyConsumer : public BodyConsumer
            {
            public:
                std::vector<uint8_t> body;
                HTTPBaseAsync *owner;
                uint32_t reading_offset;

                DefaultBodyConsumer(HTTPBaseAsync *_owner) : owner(_owner), reading_offset(0)
                {
                }

                uint32_t body_write(const uint8_t *data, uint32_t size) override
                {
                    if (size == 0)
                        return 0;
                    body.insert(body.end(), data, data + size);
                    return size;
                }

                void body_write_start() override
                {
                    body.clear();
                    reading_offset = 0;
                }

                void body_read_start() override
                {
                    reading_offset = 0;
                }

                int32_t body_read_get_size() override
                {
                    return (int32_t)body.size();
                }

                uint32_t body_read(uint8_t *buffer, uint32_t max_size) override
                {
                    if (reading_offset >= body.size())
                        return 0;
                    uint32_t to_read = max_size;
                    if (reading_offset + to_read > body.size())
                        to_read = (uint32_t)(body.size() - reading_offset);
                    memcpy(buffer, body.data() + reading_offset, to_read);
                    reading_offset += to_read;
                    return to_read;
                }
            };
            return std::make_shared<DefaultBodyConsumer>(this);
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif
