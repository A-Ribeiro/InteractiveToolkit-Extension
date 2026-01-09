#pragma once

// opaque structs
struct mbedtls_x509_crt;
struct mbedtls_x509_crl;

#include "TLSUtils.h"

namespace TLS
{
    class SSLContext;

    class CertificateChain
    {
        bool initialized;

        std::unique_ptr<mbedtls_x509_crt> x509_crt;
        std::unique_ptr<mbedtls_x509_crl> x509_crl;

        CertificateChain();
    public:

        bool isInitialized() const;

        void initialize_structures();
        void release_structures();

        ~CertificateChain();

        bool addCertificate(const uint8_t* data, size_t length, bool add_all_certificates_is_required = true);
        bool addCertificateRevokationList(const uint8_t* data, size_t length, bool add_all_crl_is_required = true);

        bool addCertificateFromFile(const char *path, bool add_all_certificates_is_required = true);
        bool addCertificateRevokationListFromFile(const char *path, bool add_all_crl_is_required = true);
        
        bool addSystemCertificates(bool add_all_certificates_is_required = false,
                                   bool add_all_crl_is_required = false);

        std::string getCertificateCommonName(int position_in_chain = 0);

        TLS_DECLARE_CREATE_SHARED(CertificateChain)

        friend class TLS::SSLContext;
    };

}
