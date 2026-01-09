#include <InteractiveToolkit-Extension/network/tls/PrivateKey.h>
#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>
#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>
#include <InteractiveToolkit-Extension/network/tls/GlobalSharedState.h>

#include <mbedtls/pk.h>

namespace TLS
{

    bool PrivateKey::isInitialized() const
    {
        return initialized;
    }

    void PrivateKey::initialize_structures()
    {
        if (!initialized)
        {
            mbedtls_pk_init(private_key_context.get());
            initialized = true;
        }
    }

    void PrivateKey::release_structures()
    {
        if (initialized)
        {
            mbedtls_pk_free(private_key_context.get());
            initialized = false;
        }
    }

    PrivateKey::PrivateKey() : initialized(false)
    {
        private_key_context = STL_Tools::make_unique<mbedtls_pk_context>();
    }

    PrivateKey::~PrivateKey()
    {
        release_structures();
    }

    bool PrivateKey::setKeyEncrypted(const uint8_t *key, size_t key_length,
                                     const char *key_decrypt_password)
    {
        initialize_structures();

        int result;

        // Check if data starts with PEM format marker
        const char *pem_begin = "-----BEGIN";
        bool is_pem = key_length >= strlen(pem_begin) && memcmp(key, pem_begin, strlen(pem_begin)) == 0;

        // Check for DER format (starts with ASN.1 SEQUENCE tag 0x30)
        bool is_der = key_length >= 2 && (key[0] == 0x30 &&
                                          key[1] >= 0x80); // DER length encoding

        // If neither PEM nor DER, cannot parse
        if (!is_pem && !is_der)
            return false;

        std::string keyStr_aux;

        // DER format (binary)
        const uint8_t *key_to_use = key;
        size_t key_to_use_length = key_length;

        if (is_pem)
        {
            // PEM format - needs null termination
            keyStr_aux = std::string((const char *)key, key_length);
            key_to_use = (const uint8_t *)keyStr_aux.c_str();
            key_to_use_length = strlen(keyStr_aux.c_str()) + 1;
        }

        size_t key_decrypt_password_length = (key_decrypt_password == nullptr) ? 0 : strlen(key_decrypt_password);

        result = GlobalSharedState::setPrimaryKey(
            this,
            (const unsigned char *)key_to_use, key_to_use_length,
            (const unsigned char *)key_decrypt_password, key_decrypt_password_length);

        // #if MBEDTLS_VERSION_MAJOR == 3
        //         result = mbedtls_pk_parse_key(private_key_context.get(),
        //                                       (const unsigned char *)key_to_use, key_to_use_length,
        //                                       (const unsigned char *)key_decrypt_password, key_decrypt_password_length,
        //                                       mbedtls_ctr_drbg_random,
        //                                       &GlobalSharedState::Instance()->ctr_drbg_context);
        // #else
        //         result = mbedtls_pk_parse_key(private_key_context.get(),
        //                                       (const unsigned char *)key_to_use, key_to_use_length,
        //                                       (const unsigned char *)key_decrypt_password, key_decrypt_password_length);
        // #endif

        if (result != 0)
            printf("Failed to load private key: %s\n", TLS::TLSUtils::errorMessageFromReturnCode(result).c_str());
        return result == 0;
    }

    bool PrivateKey::setKeyNotEncrypted(const uint8_t *key, size_t key_length)
    {
        return setKeyEncrypted(key, key_length, nullptr);
    }

    bool PrivateKey::setKeyEncryptedFromFile(const char *key_path, const char *key_decrypt_password)
    {
        auto key_file = ITKCommon::FileSystem::File::FromPath(key_path);
        if (!key_file.isFile)
            return false;
        Platform::ObjectBuffer key_data;
        if (!key_file.readContentToObjectBuffer(&key_data))
            return false;
        return setKeyEncrypted(key_data.data, key_data.size, key_decrypt_password);
    }

    bool PrivateKey::setKeyNotEncryptedFromFile(const char *key_path)
    {
        auto key_file = ITKCommon::FileSystem::File::FromPath(key_path);
        if (!key_file.isFile)
            return false;
        Platform::ObjectBuffer key_data;
        if (!key_file.readContentToObjectBuffer(&key_data))
            return false;
        return setKeyNotEncrypted(key_data.data, key_data.size);
    }

}
