#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>

#include <mbedtls/error.h>

namespace TLS
{
    std::string TLSUtils::errorMessageFromReturnCode(int errnum)
    {
        char buffer[1024];
        mbedtls_strerror(errnum, buffer, sizeof(buffer));
        return buffer;
    }
}
