#include <InteractiveToolkit-Extension/network/HTTPRequest.h>
#include <InteractiveToolkit/ITKCommon/StringUtil.h>

namespace ITKExtension
{
    namespace Network
    {

        bool HTTPRequest::read_first_line(const std::string &firstLine)
        {
            auto parts = ITKCommon::StringUtil::tokenizer(firstLine, " ");
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
                    printf("[HTTPRequest] Unsupported HTTP Method: %s\n", method.c_str());
                    return false;
                }
                return true;
            }
            return false;
        }
        std::string HTTPRequest::mount_first_line()
        {
            return ITKCommon::PrintfToStdString("%s %s %s", method.c_str(), path.c_str(), http_version.c_str());
        }

        void HTTPRequest::clear()
        {
            headers.clear();
            method.clear();
            path.clear();
            http_version.clear();
            body.clear();
        }

        HTTPRequest &HTTPRequest::setDefault(
            std::string host,
            std::string method,
            std::string path,
            std::string http_version)
        {
            headers.clear();
            headers["Host"] = host;
            // Don't set "Connection: close" - let HTTP/1.1 keep-alive be the default
            // This prevents servers from closing the connection prematurely
            headers["Connection"] = "keep-alive";
            headers["User-Agent"] = "ITK-HTTP-Client/1.0";

            this->method = method;
            this->path = path;
            this->http_version = http_version;

            this->body.clear();

            return *this;
        }



    }
}