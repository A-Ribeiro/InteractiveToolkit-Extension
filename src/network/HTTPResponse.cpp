#include <InteractiveToolkit-Extension/network/HTTPResponse.h>
#include <InteractiveToolkit/ITKCommon/StringUtil.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {

        bool HTTPResponse::read_first_line(const std::string &firstLine)
        {
            // Parse the first line of the HTTP response: HTTP/1.1 200 OK
            auto parts = ITKCommon::StringUtil::tokenizer(firstLine, " ");
            if (parts.size() >= 3 && ITKCommon::StringUtil::startsWith(parts[0], "HTTP/"))
            {
                http_version = parts[0];
                if (sscanf(parts[1].c_str(), "%i", &status_code) != 1)
                {
                    printf("[HTTPResponse] Invalid status code format: %s\n", parts[1].c_str());
                    return false;
                }
                // reason_phrase = firstLine.substr(parts[1].size() + parts[0].size() + 2);
                reason_phrase = getReasonPhraseFromStatusCode(status_code);
                return true;
            }
            return false;
        }
        std::string HTTPResponse::mount_first_line()
        {
            return ITKCommon::PrintfToStdString("%s %d %s", http_version.c_str(), status_code, reason_phrase.c_str());
        }

        const std::string &HTTPResponse::getReasonPhraseFromStatusCode(int status_code) const
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

        void HTTPResponse::clear()
        {
            headers.clear();
            status_code = 0;
            reason_phrase.clear();
            http_version.clear();
            body.clear();
        }

        HTTPResponse &HTTPResponse::setDefault(
            int status_code,
            std::string http_version)
        {
            headers.clear();
            // Set Connection: keep-alive to allow connection reuse
            // This prevents the server from closing after the response
            headers["Connection"] = "keep-alive";

            // Set status code and reason phrase from HTTP status code list

            this->status_code = status_code;
            this->reason_phrase = getReasonPhraseFromStatusCode(status_code);
            this->http_version = http_version;

            this->body.clear();

            return *this;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif
