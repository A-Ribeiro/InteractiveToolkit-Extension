#pragma once

#include <InteractiveToolkit/Platform/SocketTCP.h>
#include "tls/SSLContext.h"
#include "tls/GlobalSharedState.h"

namespace ITKExtension
{
    namespace Network
    {
        class SocketTCP_SSL : public Platform::SocketTCP
        {
        public:
            TLS::SSLContext sslContext;

            // deleted copy constructor and assign operator, to avoid copy...
            SocketTCP_SSL(const SocketTCP_SSL &v) = delete;
            SocketTCP_SSL &operator=(const SocketTCP_SSL &v) = delete;

            SocketTCP_SSL() = default;
            ~SocketTCP_SSL() = default;

            // @param hostname_or_common_name: set the expected hostname for server certificate verification
            //          - The Common Name (e.g. server FQDN or YOUR name) parameter of the server certificate
            //          - The Host passed through the HTTP header
            bool configureAsClient(std::shared_ptr<TLS::CertificateChain> certificate_chain, const char *hostname_or_common_name, bool verify_peer = true);
            bool configureAsServer(std::shared_ptr<TLS::CertificateChain> certificate_chain, std::shared_ptr<TLS::PrivateKey> private_key, bool verify_peer = false);


            Platform::SocketResult doHandshake();
            Platform::SocketResult write_buffer(const uint8_t *data, uint32_t size, uint32_t *write_feedback, bool block_until_write_size = false);
            Platform::SocketResult read_buffer(uint8_t *data, uint32_t size, uint32_t *read_feedback, bool block_until_read_size = false);
            void close();

            TLS_DECLARE_CREATE_SHARED(SocketTCP_SSL)
        };
    }
}
