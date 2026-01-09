#pragma once

#include "HTTPBase.h"

namespace ITKExtension
{
    namespace Network
    {

        class HTTPResponse : public HTTPBase
        {
        protected:
            bool read_first_line(const std::string &firstLine);
            std::string mount_first_line();

            const std::string &getReasonPhraseFromStatusCode(int status_code) const;

        public:
            int status_code;
            std::string reason_phrase;
            std::string http_version;

            void clear();

            HTTPResponse &setDefault(
                int status_code = 200,
                std::string http_version = "HTTP/1.1");

            ITK_DECLARE_CREATE_SHARED(HTTPResponse)
        };

    }
}