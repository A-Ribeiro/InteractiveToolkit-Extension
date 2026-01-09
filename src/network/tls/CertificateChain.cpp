#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>
#include <InteractiveToolkit-Extension/network/tls/CertificateChain.h>
#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>
#include <InteractiveToolkit-Extension/network/tls/SystemCertificates.h>

#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_crl.h>

#include <mbedtls/oid.h>

namespace TLS
{

    bool CertificateChain::isInitialized() const
    {
        return initialized;
    }

    void CertificateChain::initialize_structures()
    {
        if (!initialized)
        {
            mbedtls_x509_crt_init(x509_crt.get());
            mbedtls_x509_crl_init(x509_crl.get());
            initialized = true;
        }
    }
    void CertificateChain::release_structures()
    {
        if (initialized)
        {
            mbedtls_x509_crl_free(x509_crl.get());
            mbedtls_x509_crt_free(x509_crt.get());
            initialized = false;
        }
    }

    CertificateChain::CertificateChain() : initialized(false)
    {
        x509_crt = STL_Tools::make_unique<mbedtls_x509_crt>();
        x509_crl = STL_Tools::make_unique<mbedtls_x509_crl>();
    }

    CertificateChain::~CertificateChain()
    {
        release_structures();
    }

    bool CertificateChain::addCertificate(const uint8_t *data, size_t length, bool add_all_certificates_is_required)
    {
        initialize_structures();
        int result;

        // Check if data starts with PEM format marker
        const char *pem_begin = "-----BEGIN";
        bool is_pem = length >= strlen(pem_begin) && memcmp(data, pem_begin, strlen(pem_begin)) == 0;
        // Check for DER format (starts with ASN.1 SEQUENCE tag 0x30)
        bool is_der = length >= 2 && (data[0] == 0x30 &&
                                      data[1] >= 0x80); // DER length encoding

        // If neither PEM nor DER, cannot parse
        if (!is_pem && !is_der)
            return !add_all_certificates_is_required; // ignore if not required

        if (is_pem)
        {
            // PEM format - needs null termination
            std::string dataStr((const char *)data, length);
            result = mbedtls_x509_crt_parse(x509_crt.get(), (const unsigned char *)dataStr.c_str(), strlen(dataStr.c_str()) + 1);
        }
        else // DER format (binary)
            result = mbedtls_x509_crt_parse(x509_crt.get(), (const unsigned char *)data, length);
        if (result < 0)
            printf("Failed to add certificate: %s\n", TLS::TLSUtils::errorMessageFromReturnCode(result).c_str());
        if (add_all_certificates_is_required)
        {
            if (result > 0)
                printf("Error: %d certificates were added, some were ignored.\n", result);
            return result == 0;
        }
        // may ignore some certificates...
        return result >= 0;
    }
    bool CertificateChain::addCertificateRevokationList(const uint8_t *data, size_t length, bool add_all_crl_is_required)
    {
        initialize_structures();
        int result;

        // Check if data starts with PEM format marker
        const char *pem_begin = "-----BEGIN";
        bool is_pem = length >= strlen(pem_begin) && memcmp(data, pem_begin, strlen(pem_begin)) == 0;

        // Check for DER format (starts with ASN.1 SEQUENCE tag 0x30)
        bool is_der = length >= 2 && (data[0] == 0x30 &&
                                      data[1] >= 0x80); // DER length encoding

        // If neither PEM nor DER, cannot parse
        if (!is_pem && !is_der)
            return !add_all_crl_is_required; // ignore if not required

        if (is_pem)
        {
            // PEM format - needs null termination
            std::string dataStr((const char *)data, length);
            result = mbedtls_x509_crl_parse(x509_crl.get(), (const unsigned char *)dataStr.c_str(), strlen(dataStr.c_str()) + 1);
        }
        else // DER format (binary)
            result = mbedtls_x509_crl_parse(x509_crl.get(), (const unsigned char *)data, length);
        if (result < 0)
            printf("Failed to add certificate: %s\n", TLS::TLSUtils::errorMessageFromReturnCode(result).c_str());
        if (add_all_crl_is_required)
        {
            if (result > 0)
                printf("Error: %d certificates were added, some were ignored.\n", result);
            return result == 0;
        }
        // may ignore some certificates...
        return result >= 0;
    }

    bool CertificateChain::addCertificateFromFile(const char *path, bool add_all_certificates_is_required)
    {
        auto file = ITKCommon::FileSystem::File::FromPath(path);
        if (!file.isFile)
            return false;
        Platform::ObjectBuffer data;
        if (!file.readContentToObjectBuffer(&data))
            return false;
        return addCertificate(data.data, data.size, add_all_certificates_is_required);
    }

    bool CertificateChain::addCertificateRevokationListFromFile(const char *path, bool add_all_crl_is_required)
    {
        auto file = ITKCommon::FileSystem::File::FromPath(path);
        if (!file.isFile)
            return false;
        Platform::ObjectBuffer data;
        if (!file.readContentToObjectBuffer(&data))
            return false;
        return addCertificateRevokationList(data.data, data.size, add_all_crl_is_required);
    }

    bool CertificateChain::addSystemCertificates(bool add_all_certificates_is_required, bool add_all_crl_is_required)
    {
        SystemCertificates sys_certs;
        return sys_certs.iterate_over_x509_certificates( //
            [this, add_all_certificates_is_required](const uint8_t *data, size_t size)
            { addCertificate(data, size, add_all_certificates_is_required); },
            [this, add_all_crl_is_required](const uint8_t *data, size_t size)
            { addCertificateRevokationList(data, size, add_all_crl_is_required); });
    }

    std::string CertificateChain::getCertificateCommonName(int position_in_chain)
    {
        mbedtls_x509_crt *x509_crt_it = x509_crt.get();

        while (position_in_chain > 0 && x509_crt_it != nullptr)
        {
            x509_crt_it = x509_crt_it->next;
            position_in_chain--;
        }

        if (x509_crt_it == nullptr)
            return "";

        // The subject field contains all the distinguished name components (CN, O, OU, C, etc.)
        const mbedtls_x509_name *name = &x509_crt_it->subject;
        // Iterate through the subject attributes
        while (name != nullptr)
        {
            // Check if this is the Common Name (CN) field
            if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &name->oid) == 0)
                return std::string((char *)name->val.p, name->val.len);
            name = name->next;
        }
        return ""; // CN not found
    }

}
