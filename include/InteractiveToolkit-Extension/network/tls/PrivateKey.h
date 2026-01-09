#pragma once

#include <mbedtls/pk.h>

#include "TLSUtils.h"

namespace TLS
{

    class PrivateKey
    {
        bool initialized;

        PrivateKey();
    public:
        mbedtls_pk_context private_key_context;

        bool isInitialized() const;

        void initialize_structures();
        void release_structures();

        ~PrivateKey();

        bool setKeyEncrypted(const uint8_t *key, size_t key_length,
                             const char *key_decrypt_password);

        bool setKeyNotEncrypted(const uint8_t *key, size_t key_length);

        bool setKeyEncryptedFromFile(const char *key_path, const char *key_decrypt_password);

        bool setKeyNotEncryptedFromFile(const char *key_path);

        TLS_DECLARE_CREATE_SHARED(PrivateKey)

    };

}
