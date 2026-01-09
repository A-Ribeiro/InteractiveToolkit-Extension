#include <InteractiveToolkit-Extension/network/SocketTCP_SSL.h>

namespace ITKExtension
{
    namespace Network
    {

        // @param hostname_or_common_name: set the expected hostname for server certificate verification
        //          - The Common Name (e.g. server FQDN or YOUR name) parameter of the server certificate
        //          - The Host passed through the HTTP header
        bool SocketTCP_SSL::handshakeAsClient(std::shared_ptr<TLS::CertificateChain> certificate_chain, const char *hostname_or_common_name, bool verify_peer)
        {
            return !sslContext.setup_called &&
                   sslContext.setupAsClient(certificate_chain, hostname_or_common_name, verify_peer) &&
                   sslContext.communicateWithThisSocket(this) &&
                   sslContext.doHandshake();
        }

        bool SocketTCP_SSL::handshakeAsServer(std::shared_ptr<TLS::CertificateChain> certificate_chain, std::shared_ptr<TLS::PrivateKey> private_key, bool verify_peer)
        {
            return !sslContext.setup_called &&
                   sslContext.setupAsServer(certificate_chain, private_key, verify_peer) &&
                   sslContext.communicateWithThisSocket(this) &&
                   sslContext.doHandshake();
        }

        bool SocketTCP_SSL::write_buffer(const uint8_t *data, uint32_t size, uint32_t *write_feedback)
        {
            return sslContext.write_buffer(data, size, write_feedback);
        }

        bool SocketTCP_SSL::read_buffer(uint8_t *data, uint32_t size, uint32_t *read_feedback, bool only_returns_if_match_exact_size)
        {
            if (only_returns_if_match_exact_size)
            {
                *read_feedback = 0;
                while (*read_feedback < size)
                {
                    if (!sslContext.read_buffer(data + *read_feedback, size - *read_feedback, read_feedback))
                        return *read_feedback == size;
                }
                return true;
            }
            return sslContext.read_buffer(data, size, read_feedback);
        }

        void SocketTCP_SSL::close()
        {
            // printf("[SocketTCP_SSL] Closing SSL SocketTCP_SSL...\n");
            sslContext.close();
            SocketTCP::close();
            // make possible to do another handshake, in case of a new connection
            sslContext.setup_called = false;
        }
    }
}
