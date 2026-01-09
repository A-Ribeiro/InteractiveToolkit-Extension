#pragma once

#include <InteractiveToolkit/EventCore/Callback.h>

#if defined(__APPLE__)
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#endif


namespace TLS
{
    class SystemCertificates
    {
#if defined(_WIN32)
        bool list_certificates(const char *store,
                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRT,
                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRL);
#elif defined(__APPLE__)
        std::string osxErrorToString(OSStatus status);
        bool list_certificates(SecTrustSettingsDomain domain,
                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRT,
                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRL);
#elif defined(__linux__) || defined(__ANDROID__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
        bool list_certificates(const char *path_to_list,
                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRT,
                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRL);
#endif
    public:
        bool iterate_over_x509_certificates(
            const EventCore::Callback<void(const uint8_t *data, size_t size)> &certificate_callback,
            const EventCore::Callback<void(const uint8_t *data, size_t size)> &certificate_revocation_list_callback);
    };
}
