#pragma once

#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_crl.h>

#include "TLSUtils.h"

namespace TLS
{

    class CertificateChain
    {
        bool initialized;

        CertificateChain();
    public:

        mbedtls_x509_crt x509_crt;
        mbedtls_x509_crl x509_crl;

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

        static std::string getCertificateCommonName(mbedtls_x509_crt *x509_crt);

        TLS_DECLARE_CREATE_SHARED(CertificateChain)

    };

}
