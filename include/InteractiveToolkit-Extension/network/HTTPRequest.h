#pragma once

#include "HTTPBase.h"

namespace ITKExtension
{
    namespace Network
    {

        class HTTPRequest : public HTTPBase
        {
        protected:
            bool read_first_line(const std::string &firstLine);
            std::string mount_first_line();

        public:
            std::string method;
            std::string path;
            std::string http_version;

            void clear();

            HTTPRequest &setDefault(
                std::string host = "subdomain.example.com",
                std::string method = "GET",
                std::string path = "/",
                std::string http_version = "HTTP/1.1");

            ITK_DECLARE_CREATE_SHARED(HTTPRequest)
        };

    }
}