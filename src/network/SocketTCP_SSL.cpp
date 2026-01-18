#include <InteractiveToolkit-Extension/network/SocketTCP_SSL.h>

namespace ITKExtension
{
    namespace Network
    {

        // @param hostname_or_common_name: set the expected hostname for server certificate verification
        //          - The Common Name (e.g. server FQDN or YOUR name) parameter of the server certificate
        //          - The Host passed through the HTTP header
        bool SocketTCP_SSL::configureAsClient(std::shared_ptr<TLS::CertificateChain> certificate_chain, const char *hostname_or_common_name, bool verify_peer)
        {
            return !sslContext.setup_called &&
                   sslContext.setupAsClient(certificate_chain, hostname_or_common_name, verify_peer) &&
                   sslContext.communicateWithThisSocket(this);
        }

        bool SocketTCP_SSL::configureAsServer(std::shared_ptr<TLS::CertificateChain> certificate_chain, std::shared_ptr<TLS::PrivateKey> private_key, bool verify_peer)
        {
            return !sslContext.setup_called &&
                   sslContext.setupAsServer(certificate_chain, private_key, verify_peer) &&
                   sslContext.communicateWithThisSocket(this);
        }

        Platform::SocketResult SocketTCP_SSL::doHandshake(const EventCore::Callback<void(const std::string &error, std::shared_ptr<TLS::Certificate> certificate)> &on_verification_error) 
        {
            return sslContext.doHandshake(on_verification_error);
        }

        Platform::SocketResult SocketTCP_SSL::write_buffer(const uint8_t *data, uint32_t size, uint32_t *write_feedback, bool block_until_write_size)
        {
            if (block_until_write_size)
            {
                *write_feedback = 0;
                while (*write_feedback < size)
                {
                    uint32_t written = 0;
                    Platform::SocketResult res = sslContext.write_buffer(data + *write_feedback, size - *write_feedback, &written);
                    if (res != Platform::SOCKET_RESULT_OK)
                    {
                        if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                        {
                            Platform::Sleep::millis(1);
                            continue;
                        }
                        return res;
                    }
                    *write_feedback += written;
                }
                return Platform::SOCKET_RESULT_OK;
            }
            return sslContext.write_buffer(data, size, write_feedback);
        }

        Platform::SocketResult SocketTCP_SSL::read_buffer(uint8_t *data, uint32_t size, uint32_t *read_feedback, bool block_until_read_size)
        {
            if (block_until_read_size)
            {
                *read_feedback = 0;
                while (*read_feedback < size)
                {
                    uint32_t readed = 0;
                    Platform::SocketResult res = sslContext.read_buffer(data + *read_feedback, size - *read_feedback, &readed);
                    if (res != Platform::SOCKET_RESULT_OK)
                    {
                        if (res == Platform::SOCKET_RESULT_WOULD_BLOCK || res == Platform::SOCKET_RESULT_TIMEOUT)
                        {
                            Platform::Sleep::millis(1);
                            continue;
                        }
                        return res;
                    }
                    *read_feedback += readed;
                }
                return Platform::SOCKET_RESULT_OK;
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
