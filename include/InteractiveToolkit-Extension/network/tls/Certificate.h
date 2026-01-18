#pragma once

// opaque structs
struct mbedtls_x509_crt;

#include "TLSUtils.h"
#include <InteractiveToolkit/ITKCommon/Date.h>
#include <InteractiveToolkit/Platform/Core/ObjectBuffer.h>

namespace TLS
{
    class SSLContext;
    class CertificateChain;

    class Certificate
    {
        std::unique_ptr<mbedtls_x509_crt> x509_crt;
        const mbedtls_x509_crt *crt_ptr;

        Certificate(const mbedtls_x509_crt *crt);
    public:

        // delete copy constructor and assignment
        Certificate(const Certificate&) = delete;
        Certificate& operator=(const Certificate&) = delete;

        Certificate(const uint8_t *data, size_t length);
        ~Certificate();

        // CN (Common Name) - typically the hostname or person's name
        // O (Organization) - company or organization name
        // OU (Organizational Unit) - department or division
        // C (Country) - two-letter country code
        // ST (State/Province) - state or province name
        // L (Locality) - city name
        // Email - email address
        std::string getSubjectDistinguishedNameString() const;

        // CN (Common Name) - typically the hostname or person's name
        // O (Organization) - company or organization name
        // OU (Organizational Unit) - department or division
        // C (Country) - two-letter country code
        // ST (State/Province) - state or province name
        // L (Locality) - city name
        // Email - email address
        std::string getIssuerDistinguishedNameString() const;

        std::string getSubjectCommonName() const;
        
        ITKCommon::Date getValidFrom() const;
        ITKCommon::Date getValidTo() const;
        std::string getFingerprintSHA256() const;
        std::string getSerialNumber() const;

        void getDEREncoded(Platform::ObjectBuffer *output) const;
        std::vector<uint8_t> getDEREncoded() const;
        std::string getPEMEncoded() const;

        std::shared_ptr<Certificate> clone() const;

        friend class TLS::CertificateChain;
        friend class TLS::SSLContext;
    };

}
