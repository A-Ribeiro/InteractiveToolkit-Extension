#include <InteractiveToolkit-Extension/network/tls/CertificateChain.h>
#include <InteractiveToolkit-Extension/network/tls/SSLContext.h>
#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>
#include <InteractiveToolkit/Platform/SocketTCP.h>
#include <InteractiveToolkit-Extension/network/tls/GlobalSharedState.h>

#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>

// compatible layer with different Mbed TLS versions
#ifndef MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET
#define MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET 0
#endif

#ifndef MBEDTLS_X509_CRT_ERROR_INFO_LIST
#define MBEDTLS_X509_CRT_ERROR_INFO_LIST                                                                     \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_EXPIRED, "MBEDTLS_X509_BADCERT_EXPIRED",                        \
                        "The certificate validity has expired")                                              \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_REVOKED, "MBEDTLS_X509_BADCERT_REVOKED",                        \
                        "The certificate has been revoked (is on a CRL)")                                    \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_CN_MISMATCH, "MBEDTLS_X509_BADCERT_CN_MISMATCH",                \
                        "The certificate Common Name (CN) does not match with the expected CN")              \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_NOT_TRUSTED, "MBEDTLS_X509_BADCERT_NOT_TRUSTED",                \
                        "The certificate is not correctly signed by the trusted CA")                         \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_NOT_TRUSTED, "MBEDTLS_X509_BADCRL_NOT_TRUSTED",                  \
                        "The CRL is not correctly signed by the trusted CA")                                 \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_EXPIRED, "MBEDTLS_X509_BADCRL_EXPIRED",                          \
                        "The CRL is expired")                                                                \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_MISSING, "MBEDTLS_X509_BADCERT_MISSING",                        \
                        "Certificate was missing")                                                           \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_SKIP_VERIFY, "MBEDTLS_X509_BADCERT_SKIP_VERIFY",                \
                        "Certificate verification was skipped")                                              \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_OTHER, "MBEDTLS_X509_BADCERT_OTHER",                            \
                        "Other reason (can be used by verify callback)")                                     \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_FUTURE, "MBEDTLS_X509_BADCERT_FUTURE",                          \
                        "The certificate validity starts in the future")                                     \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_FUTURE, "MBEDTLS_X509_BADCRL_FUTURE",                            \
                        "The CRL is from the future")                                                        \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_KEY_USAGE, "MBEDTLS_X509_BADCERT_KEY_USAGE",                    \
                        "Usage does not match the keyUsage extension")                                       \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_EXT_KEY_USAGE, "MBEDTLS_X509_BADCERT_EXT_KEY_USAGE",            \
                        "Usage does not match the extendedKeyUsage extension")                               \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_NS_CERT_TYPE, "MBEDTLS_X509_BADCERT_NS_CERT_TYPE",              \
                        "Usage does not match the nsCertType extension")                                     \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_BAD_MD, "MBEDTLS_X509_BADCERT_BAD_MD",                          \
                        "The certificate is signed with an unacceptable hash.")                              \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_BAD_PK, "MBEDTLS_X509_BADCERT_BAD_PK",                          \
                        "The certificate is signed with an unacceptable PK alg (eg RSA vs ECDSA).")          \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCERT_BAD_KEY, "MBEDTLS_X509_BADCERT_BAD_KEY",                        \
                        "The certificate is signed with an unacceptable key (eg bad curve, RSA too short).") \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_BAD_MD, "MBEDTLS_X509_BADCRL_BAD_MD",                            \
                        "The CRL is signed with an unacceptable hash.")                                      \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_BAD_PK, "MBEDTLS_X509_BADCRL_BAD_PK",                            \
                        "The CRL is signed with an unacceptable PK alg (eg RSA vs ECDSA).")                  \
    X509_CRT_ERROR_INFO(MBEDTLS_X509_BADCRL_BAD_KEY, "MBEDTLS_X509_BADCRL_BAD_KEY",                          \
                        "The CRL is signed with an unacceptable key (eg bad curve, RSA too short).")

#endif

namespace TLS
{

    bool SSLContext::isInitialized() const
    {
        return initialized;
    }

    void SSLContext::initialize_structures()
    {
        if (!initialized)
        {
            mbedtls_ssl_init(ssl_context.get());
            mbedtls_ssl_config_init(ssl_config.get());
            initialized = true;
        }
    }
    void SSLContext::release_structures()
    {
        if (initialized)
        {
            mbedtls_ssl_config_free(ssl_config.get());
            mbedtls_ssl_free(ssl_context.get());
            initialized = false;
            is_client = false;
            is_server = false;
            // setup_called = false; // use as a global flag to the containing socket
            handshake_done = false;
            private_key = nullptr;
            certificate_chain = nullptr;
            
            // Free peer certificate copy if exists
            peer_cert_copy.reset();
        }
    }

    SSLContext::SSLContext() : initialized(false), is_client(false), is_server(false), setup_called(false), handshake_done(false),
                               private_key(nullptr), certificate_chain(nullptr)
    {
        ssl_context = STL_Tools::make_unique<mbedtls_ssl_context>();
        ssl_config = STL_Tools::make_unique<mbedtls_ssl_config>();
    }

    SSLContext::~SSLContext()
    {
        release_structures();
    }

    bool SSLContext::setupAsClient(std::shared_ptr<CertificateChain> certificate_chain, const char *hostname_or_common_name, bool verify_peer)
    {
        if (this->certificate_chain != nullptr)
            return false;
        initialize_structures();

        is_client = true;
        setup_called = true;
        this->certificate_chain = certificate_chain;

        auto ssl_config_ptr = ssl_config.get();

        int result = mbedtls_ssl_config_defaults(ssl_config_ptr,
                                                 MBEDTLS_SSL_IS_CLIENT,
                                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                                 MBEDTLS_SSL_PRESET_DEFAULT);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        //         // random number generator explicitly is required before Mbed TLS 4.x.x
        // #if (MBEDTLS_VERSION_MAJOR < 4)
        //         mbedtls_ssl_conf_rng(ssl_config_ptr, mbedtls_ctr_drbg_random, &GlobalSharedState::Instance()->ctr_drbg_context);
        // #endif
        GlobalSharedState::setSslRng(this);

        // peer verification mode
        mbedtls_ssl_conf_authmode(ssl_config_ptr, verify_peer ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_NONE);

        // Set custom verification callback to capture peer certificate
        mbedtls_ssl_conf_verify(ssl_config_ptr,
            [](void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags) -> int {
                SSLContext *ssl_ctx = static_cast<SSLContext*>(ctx);
                // Capture the leaf certificate (depth == 0)
                if (depth == 0 && crt != nullptr && crt->raw.p != nullptr && crt->raw.len > 0) {
                    // Create a copy of the certificate
                    ssl_ctx->peer_cert_copy = std::make_shared<Certificate>(crt->raw.p, crt->raw.len);
                }
                // Return 0 to continue with default verification
                return 0;
            }, this);

        mbedtls_ssl_conf_ca_chain(ssl_config_ptr, certificate_chain->x509_crt.get(), certificate_chain->x509_crl.get());

        auto ssl_context_ptr = ssl_context.get();

        result = mbedtls_ssl_setup(ssl_context_ptr, ssl_config_ptr);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        // Set the hostname that is used for peer verification and sent via SNI if it is supported
        // Only for clients
        result = mbedtls_ssl_set_hostname(ssl_context_ptr, hostname_or_common_name);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        return true;
    }

    bool SSLContext::setupAsServer(std::shared_ptr<CertificateChain> certificate_chain, std::shared_ptr<PrivateKey> private_key, bool verify_peer)
    {
        if (this->certificate_chain != nullptr)
            return false;
        initialize_structures();

        is_server = true;
        setup_called = true;
        this->certificate_chain = certificate_chain;
        this->private_key = private_key;

        auto ssl_config_ptr = ssl_config.get();

        int result = mbedtls_ssl_config_defaults(ssl_config_ptr,
                                                 MBEDTLS_SSL_IS_SERVER,
                                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                                 MBEDTLS_SSL_PRESET_DEFAULT);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        //         // random number generator explicitly is required before Mbed TLS 4.x.x
        // #if (MBEDTLS_VERSION_MAJOR < 4)
        //         mbedtls_ssl_conf_rng(ssl_config_ptr, mbedtls_ctr_drbg_random, &GlobalSharedState::Instance()->ctr_drbg_context);
        // #endif
        GlobalSharedState::setSslRng(this);

        // peer verification mode
        mbedtls_ssl_conf_authmode(ssl_config_ptr, verify_peer ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_NONE);

        // certificate chain and private key
        //   -> load CA chain, except the first
        //   -> set the 1st CA certificate as own certificate (server certificate)
        auto cert_chain = certificate_chain->x509_crt.get();
        if (cert_chain->next != nullptr)
            mbedtls_ssl_conf_ca_chain(ssl_config_ptr, cert_chain->next, nullptr);
        result = mbedtls_ssl_conf_own_cert(ssl_config_ptr, cert_chain, private_key->private_key_context.get());
        if (result != 0)
        {
            printf("Error loading server certificate: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        result = mbedtls_ssl_setup(ssl_context.get(), ssl_config_ptr);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        return true;
    }

    bool SSLContext::communicateWithThisSocket(Platform::SocketTCP *socket)
    {
        if (this->certificate_chain == nullptr)
            return false;

        mbedtls_ssl_set_bio(
            ssl_context.get(),
            socket,
            [](void *context, const unsigned char *data, std::size_t size) -> int
            {
                if (size == 0)
                    return 0;
                Platform::SocketTCP *tcp_socket = static_cast<Platform::SocketTCP *>(context);
                uint32_t write_feedback;
                Platform::SocketResult res = tcp_socket->Platform::SocketTCP::write_buffer(data, (uint32_t)size, &write_feedback);
                if (res != Platform::SOCKET_RESULT_OK)
                {
                    if (res == Platform::SOCKET_RESULT_TIMEOUT)
                        return MBEDTLS_ERR_SSL_TIMEOUT;
                    // only non-blocking sockets can return WOULD_BLOCK
                    if (res == Platform::SOCKET_RESULT_WOULD_BLOCK)
                    {
                        if (write_feedback == 0)
                            return MBEDTLS_ERR_SSL_WANT_WRITE;
                        // partial write
                        return (int)write_feedback;
                    }
                    return MBEDTLS_ERR_NET_CONN_RESET;
                }
                return (int)write_feedback;
            },
            [](void *context, unsigned char *data, std::size_t size) -> int
            {
                if (size == 0)
                    return 0;
                Platform::SocketTCP *tcp_socket = static_cast<Platform::SocketTCP *>(context);
                uint32_t read_feedback;
                Platform::SocketResult res = tcp_socket->Platform::SocketTCP::read_buffer(data, (uint32_t)size, &read_feedback);
                if (res != Platform::SOCKET_RESULT_OK)
                {
                    if (res == Platform::SOCKET_RESULT_TIMEOUT)
                        return MBEDTLS_ERR_SSL_TIMEOUT;
                    // only non-blocking sockets can return WOULD_BLOCK
                    if (res == Platform::SOCKET_RESULT_WOULD_BLOCK)
                        return MBEDTLS_ERR_SSL_WANT_READ;
                    return MBEDTLS_ERR_NET_CONN_RESET;
                }
                return (int)read_feedback;
            },
            nullptr
            // [](void *context, unsigned char *data, std::size_t size, uint32_t timeout_ms) -> int
            // {
            //     Platform::SocketTCP *tcp_socket = static_cast<Platform::SocketTCP *>(context);
            //     uint32_t read_feedback;
            //     tcp_socket->Platform::SocketTCP::setReadTimeout(timeout_ms);
            //     Platform::SocketResult res = tcp_socket->Platform::SocketTCP::read_buffer(data, (uint32_t)size, &read_feedback);
            //     if (res != Platform::SOCKET_RESULT_OK)
            //     {
            //         if (res == Platform::SOCKET_RESULT_TIMEOUT)
            //             return MBEDTLS_ERR_SSL_TIMEOUT;
            //         // only non-blocking sockets can return WOULD_BLOCK
            //         if (res == Platform::SOCKET_RESULT_WOULD_BLOCK)
            //             return MBEDTLS_ERR_SSL_WANT_READ;
            //         return MBEDTLS_ERR_NET_CONN_RESET;
            //     }
            //     return (int)read_feedback;
            // }
        );
        return true;
    }

    Platform::SocketResult SSLContext::doHandshake(const EventCore::Callback<void(const std::string &error, std::shared_ptr<Certificate> certificate)> &on_verification_error)
    {
        if (this->handshake_done)
            return Platform::SOCKET_RESULT_OK;
        if (this->certificate_chain == nullptr)
            return Platform::SOCKET_RESULT_ERROR;

        int result;
        bool should_retry;

        auto ssl_context_ptr = this->ssl_context.get();

        do
        {
            result = mbedtls_ssl_handshake(ssl_context_ptr);
            should_retry = result != 0 &&
                           (result == MBEDTLS_ERR_SSL_WANT_READ ||
                            result == MBEDTLS_ERR_SSL_WANT_WRITE ||
                            result == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
                            result == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS ||
                            // result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA ||
                            result == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET);
            // if (result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA)
            //     mbedtls_ssl_read_early_data(&ssl_context, ...);
            if (result == MBEDTLS_ERR_SSL_WANT_READ ||
                result == MBEDTLS_ERR_SSL_WANT_WRITE)
                return Platform::SOCKET_RESULT_WOULD_BLOCK;
            if (result == MBEDTLS_ERR_SSL_TIMEOUT)
                return Platform::SOCKET_RESULT_TIMEOUT;
            if (should_retry)
                Platform::Sleep::millis(1);
        } while (should_retry);

        if (result != 0)
        {
            printf("Error during TLS handshake: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());

            // print verification errors
            if (result == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
            {
                auto verifyResult = mbedtls_ssl_get_verify_result(ssl_context_ptr);
                std::string errors;

#define X509_CRT_ERROR_INFO(error_code, error_code_str, human_readable_string) \
    if (verifyResult & error_code)                                             \
        errors += " - " human_readable_string "\n";
                MBEDTLS_X509_CRT_ERROR_INFO_LIST
#undef X509_CRT_ERROR_INFO

                if (!errors.empty())
                    errors.resize(errors.size() - 1);
                printf("TLS certificate verification failed:\n%s\n", errors.c_str());

                if (on_verification_error != nullptr) {
                    if (peer_cert_copy != nullptr)
                        on_verification_error(errors, peer_cert_copy);
                    else {
                        printf("Warning: Could not retrieve peer certificate for verification error callback\n");
                        // on_verification_error(errors, nullptr);
                    }
                }
            }

            release_structures();
            return Platform::SOCKET_RESULT_ERROR;
        }

        handshake_done = true;
        return Platform::SOCKET_RESULT_OK;
    }

    void SSLContext::close()
    {
        if (!this->handshake_done)
            return;

        int result = mbedtls_ssl_close_notify(ssl_context.get());

        if (result != 0)
        {
            if (result != MBEDTLS_ERR_SSL_WANT_READ &&
                result != MBEDTLS_ERR_SSL_WANT_WRITE &&
                result != MBEDTLS_ERR_NET_CONN_RESET)
                printf("Failed to notify TLS peer about connection close: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
        }

        release_structures();
    }

    std::string SSLContext::getUsedCiphersuite() const
    {
        if (!this->handshake_done)
            return "";
        const char *cipersuiteName = mbedtls_ssl_get_ciphersuite(ssl_context.get());
        if (cipersuiteName)
            return cipersuiteName;
        return "";
    }

    Platform::SocketResult SSLContext::write_buffer(const uint8_t *data, uint32_t size, uint32_t *write_feedback)
    {
        *write_feedback = 0;
        if (!this->handshake_done)
            return Platform::SOCKET_RESULT_ERROR;

        int result;
        bool should_retry;

        uint32_t current_pos = 0;
        *write_feedback = current_pos;

        auto ssl_context_ptr = this->ssl_context.get();

        while (current_pos < size)
        {
            do
            {
                result = mbedtls_ssl_write(ssl_context_ptr,
                                           (const unsigned char *)&data[current_pos],
                                           size - current_pos);
                should_retry = result < 0 &&
                               (result == MBEDTLS_ERR_SSL_WANT_READ ||
                                result == MBEDTLS_ERR_SSL_WANT_WRITE ||
                                result == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
                                result == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET ||
                                result == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS // ||
                                                                             // result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA
                               );
                // if (result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA)
                //     mbedtls_ssl_read_early_data(&ssl_context, ...);
                if (result == MBEDTLS_ERR_SSL_WANT_READ ||
                    result == MBEDTLS_ERR_SSL_WANT_WRITE)
                    return Platform::SOCKET_RESULT_WOULD_BLOCK;
                if (result == MBEDTLS_ERR_SSL_TIMEOUT)
                    return Platform::SOCKET_RESULT_TIMEOUT;
                if (should_retry)
                    Platform::Sleep::millis(1);
            } while (should_retry);

            if (result == 0 || result == MBEDTLS_ERR_NET_CONN_RESET)
            {
                // connection was closed
                printf("TLS connection was closed during write.\n");
                release_structures();
                return Platform::SOCKET_RESULT_CLOSED;
            }
            else if (result < 0)
            {
                printf("Error writing TLS data: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
                release_structures();
                return Platform::SOCKET_RESULT_ERROR;
            }

            current_pos += (uint32_t)result;
            *write_feedback = current_pos;
        }

        return Platform::SOCKET_RESULT_OK;
    }

    Platform::SocketResult SSLContext::read_buffer(uint8_t *data, uint32_t size, uint32_t *read_feedback)
    {
        *read_feedback = 0;

        if (!this->handshake_done)
            return Platform::SOCKET_RESULT_ERROR;

        int result;
        bool should_retry;

        auto ssl_context_ptr = this->ssl_context.get();

        do
        {
            result = mbedtls_ssl_read(ssl_context_ptr, (unsigned char *)data, size);
            should_retry = result < 0 &&
                           (result == MBEDTLS_ERR_SSL_WANT_READ ||
                            result == MBEDTLS_ERR_SSL_WANT_WRITE ||
                            result == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
                            result == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET ||
                            result == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS // ||
                                                                         // result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA
                           );
            // if (result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA)
            //     mbedtls_ssl_read_early_data(&ssl_context, ...);
            if (result == MBEDTLS_ERR_SSL_WANT_READ ||
                result == MBEDTLS_ERR_SSL_WANT_WRITE)
                return Platform::SOCKET_RESULT_WOULD_BLOCK;
            if (result == MBEDTLS_ERR_SSL_TIMEOUT)
                return Platform::SOCKET_RESULT_TIMEOUT;
            if (should_retry)
                Platform::Sleep::millis(1);
        } while (should_retry);

        if (result == 0 || result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || result == MBEDTLS_ERR_NET_CONN_RESET)
        {
            printf("TLS connection was closed during read.\n");
            release_structures();
            return Platform::SOCKET_RESULT_CLOSED;
        }
        else if (result < 0)
        {
            printf("Error reading TLS data: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return Platform::SOCKET_RESULT_ERROR;
        }

        *read_feedback = (uint32_t)result;

        return Platform::SOCKET_RESULT_OK;
    }
}
