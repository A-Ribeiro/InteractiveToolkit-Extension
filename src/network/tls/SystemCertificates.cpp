#include <InteractiveToolkit-Extension/network/tls/SystemCertificates.h>

#if defined(_WIN32)
#include <wincrypt.h>
#elif defined(__APPLE__)
#include <Availability.h>
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <InteractiveToolkit/ITKCommon/FileSystem/Directory.h>
#include <InteractiveToolkit/Platform/Core/SocketUtils.h>

namespace TLS
{

#if defined(_WIN32)
    bool SystemCertificates::list_certificates(const char *store,
                                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRT,
                                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRL)
    {
        HCERTSTORE systemStore = CertOpenSystemStoreA(0, store);
        if (!systemStore)
        {
            printf("ERROR on open Windows certificate store: %s\n", Platform::SocketUtils::getLastErrorMessage().c_str());
            return false;
        }

        if (onCRT != nullptr)
        {
            const CERT_CONTEXT *cert{};
            while (cert = CertEnumCertificatesInStore(systemStore, cert))
            {
                // could fail, keep reading next
                if (cert->dwCertEncodingType == X509_ASN_ENCODING)
                    onCRT((const uint8_t *)cert->pbCertEncoded, (size_t)cert->cbCertEncoded);
            }
        }

        if (onCRL != nullptr)
        {
            const CRL_CONTEXT *crl{};
            while (crl = CertEnumCRLsInStore(systemStore, crl))
            {
                // could fail, keep reading next
                if (crl->dwCertEncodingType == X509_ASN_ENCODING)
                    onCRL((const uint8_t *)crl->pbCrlEncoded, (size_t)crl->cbCrlEncoded);
            }
        }

        if (!CertCloseStore(systemStore, 0))
            printf("ERROR on close Windows certificate store: %s\n", Platform::SocketUtils::getLastErrorMessage().c_str());

        return true;
    }
#elif defined(__APPLE__)
    std::string SystemCertificates::osxErrorToString(OSStatus status)
    {
        CFStringRef nativeString = SecCopyErrorMessageString(status, nullptr);
        std::string string(CFStringGetCStringPtr(nativeString, kCFStringEncodingUTF8));
        CFRelease(nativeString);
        return string;
    }
    bool SystemCertificates::list_certificates(SecTrustSettingsDomain domain,
                                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRT,
                                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRL)
    {
        CFArrayRef cert_list{};
        OSStatus status = SecTrustSettingsCopyCertificates(domain, &cert_list);

        if (status == errSecNoTrustSettings)
            return true;

        if (status != errSecSuccess)
        {
            printf("ERROR on load system certificate: %s\n", osxErrorToString(status).c_str());
            return false;
        }

        if (onCRT != nullptr)
        {
            for (CFIndex i = 0; i < CFArrayGetCount(cert_list); ++i)
            {
                const void *cert = CFArrayGetValueAtIndex(cert_list, i);
                CFDataRef cert_data{};

                OSStatus result = SecItemExport(cert, kSecFormatX509Cert, 0, nullptr, &cert_data);
                if (result != errSecSuccess)
                {
                    CFRelease(cert_data);
                    CFRelease(cert_list);

                    printf("ERROR on load system certificate: %s\n", osxErrorToString(result).c_str());
                    return false;
                }

                // could fail, keep reading next
                onCRT((const uint8_t *)CFDataGetBytePtr(cert_data), (size_t)CFDataGetLength(cert_data));

                CFRelease(cert_data);
            }
        }

        CFRelease(cert_list);
        return true;
    }
#elif defined(__linux__) || defined(__ANDROID__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    bool SystemCertificates::list_certificates(const char *path_to_list,
                                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRT,
                                               const EventCore::Callback<void(const uint8_t *data, size_t size)> &onCRL)
    {
        if (!ITKCommon::Path::isDirectory(path_to_list))
            return true;
        if (onCRT != nullptr)
        {
            Platform::ObjectBuffer file_content;
            for (auto &file : ITKCommon::FileSystem::Directory(path_to_list))
            {
                if (file.isDirectory)
                    continue;
                // skip files larger than 10 MB
                if (file.size >= 10 * 1024 * 1024)
                {
                    printf("[SystemCertificates] Skipping file %s (size=%llu greater than 10MB)\n", file.full_path.c_str(), (unsigned long long)file.size);
                    continue;
                }
                if (file.readContentToObjectBuffer(&file_content))
                    // if (file_content.size > 27 &&
                    //     memcmp(file_content.data, "-----BEGIN CERTIFICATE-----", 27) == 0)
                    if (file_content.size > 10)
                    {
                        // Check for PEM format
                        const char *pem_begin = "-----BEGIN";
                        bool is_pem = (file_content.size >= 10 &&
                                       memcmp(file_content.data, pem_begin, 10) == 0);

                        // Check for DER format (starts with ASN.1 SEQUENCE tag 0x30)
                        bool is_der = (file_content.data[0] == 0x30 &&
                                       file_content.data[1] >= 0x80); // DER length encoding

                        if (is_pem || is_der)
                            onCRT((const uint8_t *)file_content.data, (size_t)file_content.size);
                    }
            }
        }
        return true;
    }
#endif
    bool SystemCertificates::iterate_over_x509_certificates(
        const EventCore::Callback<void(const uint8_t *data, size_t size)> &certificate_callback,
        const EventCore::Callback<void(const uint8_t *data, size_t size)> &certificate_revocation_list_callback)
    {
#if defined(_WIN32)
        return list_certificates("ROOT", certificate_callback, certificate_revocation_list_callback) &&
               list_certificates("CA", certificate_callback, certificate_revocation_list_callback);
#elif defined(__APPLE__)
        return list_certificates(kSecTrustSettingsDomainUser, certificate_callback, certificate_revocation_list_callback) &&
               list_certificates(kSecTrustSettingsDomainAdmin, certificate_callback, certificate_revocation_list_callback) &&
               list_certificates(kSecTrustSettingsDomainSystem, certificate_callback, certificate_revocation_list_callback);
#elif defined(__linux__) || defined(__ANDROID__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
        const char *path_list[] = {
#if defined(__linux__)
            "/etc/ssl/",
            "/etc/ssl/certs/",
            "/etc/pki/ca-trust/extracted/pem/",
            "/etc/pki/tls/",
            "/etc/pki/tls/certs/",
#elif defined(__ANDROID__)
            "/system/etc/security/cacerts/",
            "/data/misc/keychain/cacerts-added/",
#elif defined(__FreeBSD__)
            "/usr/local/share/certs",
#elif defined(__OpenBSD__)
            "/etc/ssl/",
#elif defined(__NetBSD__)
            "/etc/openssl/certs",
#endif
            nullptr};
        bool result = true;
        for (const char **path = path_list; *path != nullptr; ++path)
        {
            result = result && list_certificates(
                                   *path,
                                   certificate_callback,
                                   certificate_revocation_list_callback);
        }
        return result;
#endif
    }

}
