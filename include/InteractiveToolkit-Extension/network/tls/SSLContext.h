#pragma once

#include "PrivateKey.h"
#include "CertificateChain.h"

#include <mbedtls/ssl.h>

#include <InteractiveToolkit/EventCore/Callback.h>

namespace Platform
{
    class SocketTCP;
}

#include "TLSUtils.h"

namespace TLS
{
    class SSLContext
    {
        bool initialized;

    public:
        bool is_client;
        bool is_server;

        bool setup_called;

        std::shared_ptr<PrivateKey> private_key;
        std::shared_ptr<CertificateChain> certificate_chain;

        bool handshake_done;

        mbedtls_ssl_context ssl_context;
        mbedtls_ssl_config ssl_config;

        bool isInitialized() const;

        void initialize_structures();
        void release_structures();

        SSLContext();
        ~SSLContext();

        // @param hostname_or_common_name: set the expected hostname for server certificate verification
        //          - The Common Name (e.g. server FQDN or YOUR name) parameter of the server certificate
        bool setupAsClient(std::shared_ptr<CertificateChain> &certificate_chain, const char *hostname_or_common_name, bool verify_peer = true);
        bool setupAsServer(std::shared_ptr<CertificateChain> &certificate_chain, std::shared_ptr<PrivateKey> &private_key, bool verify_peer = false);

        bool communicateWithThisSocket(Platform::SocketTCP *socket);

        bool doHandshake();

        void close();

        std::string getUsedCiphersuite() const;

        bool write_buffer(const uint8_t *data, uint32_t size, uint32_t *write_feedback);
        bool read_buffer(uint8_t *data, uint32_t size, uint32_t *read_feedback);

        TLS_DECLARE_CREATE_SHARED(SSLContext)
    };
}
