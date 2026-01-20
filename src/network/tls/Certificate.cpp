#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>
#include <InteractiveToolkit-Extension/network/tls/Certificate.h>
#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>

#include <InteractiveToolkit-Extension/hashing/SHA256.h>

#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_crl.h>

// #include <mbedtls/sha256.h>
#include <mbedtls/pem.h>

#include <mbedtls/oid.h>

namespace TLS
{
    Certificate::Certificate(const uint8_t *data, size_t length)
    {
        x509_crt = STL_Tools::make_unique<mbedtls_x509_crt>();
        mbedtls_x509_crt_init(x509_crt.get());
        crt_ptr = x509_crt.get();

        const char *pem_begin = "-----BEGIN";
        bool is_pem = length >= strlen(pem_begin) && memcmp(data, pem_begin, strlen(pem_begin)) == 0;

        int ret;
        if (is_pem)
        {
            // PEM format - needs null termination
            std::string dataStr((const char *)data, length);
            ret = mbedtls_x509_crt_parse(x509_crt.get(), (const unsigned char *)dataStr.c_str(), strlen(dataStr.c_str()) + 1);
        }
        else // DER format (binary)
            ret = mbedtls_x509_crt_parse_der(x509_crt.get(), (const unsigned char *)data, length);
    
        if (ret != 0)
            printf("Failed to parse certificate: %s\n", TLS::TLSUtils::errorMessageFromReturnCode(ret).c_str());
        
    }

    Certificate::Certificate(const mbedtls_x509_crt *crt)
    {
        crt_ptr = crt;
    }

    Certificate::~Certificate()
    {
        if (x509_crt != nullptr)
            mbedtls_x509_crt_free(x509_crt.get());
        x509_crt.reset();
        crt_ptr = nullptr;
    }

    std::string Certificate::getSubjectDistinguishedNameString() const
    {
        std::vector<char> cn_buf(64*1024);
        int ret = mbedtls_x509_dn_gets(cn_buf.data(), cn_buf.size(), &crt_ptr->subject);
        if (ret < 0)
            return "";
        return std::string(cn_buf.data());
    }

    std::string Certificate::getIssuerDistinguishedNameString() const
    {
        std::vector<char> issuer_buf(64*1024);
        int ret = mbedtls_x509_dn_gets(issuer_buf.data(), issuer_buf.size(), &crt_ptr->issuer);
        if (ret < 0)
            return "";
        return std::string(issuer_buf.data());
    }

    std::string Certificate::getSubjectCommonName() const
    {
        // The subject field contains all the distinguished name components (CN, O, OU, C, etc.)
        const mbedtls_x509_name *name = &crt_ptr->subject;
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

    ITKCommon::Date Certificate::getValidFrom() const
    {
        auto timespec = ITKCommon::Date(crt_ptr->valid_from.year,
                                        crt_ptr->valid_from.mon,
                                        0,
                                        crt_ptr->valid_from.day,
                                        crt_ptr->valid_from.hour,
                                        crt_ptr->valid_from.min,
                                        crt_ptr->valid_from.sec,
                                        0)
                            .toTimespecUTC();
        return ITKCommon::Date::FromTimeSpecUTC(timespec);
    }

    ITKCommon::Date Certificate::getValidTo() const
    {
        auto timespec = ITKCommon::Date(crt_ptr->valid_to.year,
                                        crt_ptr->valid_to.mon,
                                        0,
                                        crt_ptr->valid_to.day,
                                        crt_ptr->valid_to.hour,
                                        crt_ptr->valid_to.min,
                                        crt_ptr->valid_to.sec,
                                        0)
                            .toTimespecUTC();
        return ITKCommon::Date::FromTimeSpecUTC(timespec);
    }

    std::string Certificate::getFingerprintSHA256() const
    {
        unsigned char fingerprint[32];
        ITKExtension::Hashing::SHA256::hash(crt_ptr->raw.p, crt_ptr->raw.len, fingerprint);

        // mbedtls_sha256_context sha256_ctx;
        // mbedtls_sha256_init(&sha256_ctx);
        // mbedtls_sha256_starts(&sha256_ctx, 0); // 0 = SHA-256, 1 = SHA-224
        // mbedtls_sha256_update(&sha256_ctx, crt_ptr->raw.p, crt_ptr->raw.len);
        // mbedtls_sha256_finish(&sha256_ctx, fingerprint);
        // mbedtls_sha256_free(&sha256_ctx);

        char fingerprint_str[32 * 3] = {0}; // 2 chars + ':' per byte
        char *ptr = fingerprint_str;
        for (size_t i = 0; i < 32; i++)
        {
            snprintf(ptr, 3, "%02X", fingerprint[i]);
            ptr += 2;
            if (i < 31)
            {
                *ptr = ':';
                ptr++;
            }
        }

        return std::string(fingerprint_str);
    }

    std::string Certificate::getSerialNumber() const
    {
        char serial_buf[128];
        int ret = mbedtls_x509_serial_gets(serial_buf, sizeof(serial_buf), &crt_ptr->serial);
        if (ret < 0)
            return "";
        return std::string(serial_buf);
    }

    void Certificate::getDEREncoded(Platform::ObjectBuffer *output) const
    {
        output->setSize(crt_ptr->raw.len);
        memcpy(output->data, crt_ptr->raw.p, crt_ptr->raw.len);
    }

    std::vector<uint8_t> Certificate::getDEREncoded() const
    {
        return std::vector<uint8_t>(crt_ptr->raw.p, crt_ptr->raw.p + crt_ptr->raw.len);
    }

    std::string Certificate::getPEMEncoded() const
    {
        std::vector<unsigned char> pem_buf(64 * 1024); // 64k cert
        size_t olen = 0;
        int ret = mbedtls_pem_write_buffer(
            "-----BEGIN CERTIFICATE-----\n",
            "-----END CERTIFICATE-----\n",
            crt_ptr->raw.p,
            crt_ptr->raw.len,
            pem_buf.data(),
            pem_buf.size(),
            &olen);
        if (ret != 0)
            return "";
        return std::string((char *)pem_buf.data(), olen);
    }

    std::shared_ptr<Certificate> Certificate::clone() const
    {
        return std::make_shared<Certificate>(crt_ptr->raw.p, crt_ptr->raw.len);
    }
}
