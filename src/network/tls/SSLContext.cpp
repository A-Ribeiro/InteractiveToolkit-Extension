#include <InteractiveToolkit-Extension/network/tls/SSLContext.h>
#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>

#include <InteractiveToolkit/Platform/SocketTCP.h>

// compatible layer with different Mbed TLS versions
#ifndef MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET
#define MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET 0
#endif

#include <InteractiveToolkit-Extension/network/tls/GlobalSharedState.h>

#include <mbedtls/net_sockets.h>

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
            mbedtls_ssl_init(&ssl_context);
            mbedtls_ssl_config_init(&ssl_config);
            initialized = true;
        }
    }
    void SSLContext::release_structures()
    {
        if (initialized)
        {
            mbedtls_ssl_config_free(&ssl_config);
            mbedtls_ssl_free(&ssl_context);
            initialized = false;
            is_client = false;
            is_server = false;
            // setup_called = false; // use as a global flag to the containing socket
            handshake_done = false;
            private_key = nullptr;
            certificate_chain = nullptr;
        }
    }

    SSLContext::SSLContext() : initialized(false), is_client(false), is_server(false), setup_called(false), handshake_done(false),
                               private_key(nullptr), certificate_chain(nullptr), ssl_context{}, ssl_config{}
    {
    }

    SSLContext::~SSLContext()
    {
        release_structures();
    }

    bool SSLContext::setupAsClient(std::shared_ptr<CertificateChain> &certificate_chain, const char *hostname_or_common_name, bool verify_peer)
    {
        if (this->certificate_chain != nullptr)
            return false;
        initialize_structures();

        is_client = true;
        setup_called = true;
        this->certificate_chain = certificate_chain;

        int result = mbedtls_ssl_config_defaults(&ssl_config,
                                                 MBEDTLS_SSL_IS_CLIENT,
                                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                                 MBEDTLS_SSL_PRESET_DEFAULT);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        // random number generator explicitly is required before Mbed TLS 4.x.x
#if (MBEDTLS_VERSION_MAJOR < 4)
        mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &GlobalSharedState::Instance()->ctr_drbg_context);
#endif

        // peer verification mode
        mbedtls_ssl_conf_authmode(&ssl_config, verify_peer ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_NONE);

        mbedtls_ssl_conf_ca_chain(&ssl_config, &certificate_chain->x509_crt, &certificate_chain->x509_crl);

        result = mbedtls_ssl_setup(&ssl_context, &ssl_config);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        // Set the hostname that is used for peer verification and sent via SNI if it is supported
        // Only for clients
        result = mbedtls_ssl_set_hostname(&ssl_context, hostname_or_common_name);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        return true;
    }

    bool SSLContext::setupAsServer(std::shared_ptr<CertificateChain> &certificate_chain, std::shared_ptr<PrivateKey> &private_key, bool verify_peer)
    {
        if (this->certificate_chain != nullptr)
            return false;
        initialize_structures();

        is_server = true;
        setup_called = true;
        this->certificate_chain = certificate_chain;
        this->private_key = private_key;

        int result = mbedtls_ssl_config_defaults(&ssl_config,
                                                 MBEDTLS_SSL_IS_SERVER,
                                                 MBEDTLS_SSL_TRANSPORT_STREAM,
                                                 MBEDTLS_SSL_PRESET_DEFAULT);
        if (result != 0)
        {
            printf("Error setting up TLS: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        // random number generator explicitly is required before Mbed TLS 4.x.x
#if (MBEDTLS_VERSION_MAJOR < 4)
        mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &GlobalSharedState::Instance()->ctr_drbg_context);
#endif

        // peer verification mode
        mbedtls_ssl_conf_authmode(&ssl_config, verify_peer ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_NONE);

        // certificate chain and private key
        //   -> load CA chain, except the first
        //   -> set the 1st CA certificate as own certificate (server certificate)
        if (certificate_chain->x509_crt.next != nullptr)
            mbedtls_ssl_conf_ca_chain(&ssl_config, certificate_chain->x509_crt.next, nullptr);
        result = mbedtls_ssl_conf_own_cert(&ssl_config, &certificate_chain->x509_crt, &private_key->private_key_context);
        if (result != 0)
        {
            printf("Error loading server certificate: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        result = mbedtls_ssl_setup(&ssl_context, &ssl_config);
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
            &ssl_context,
            socket,
            [](void *context, const unsigned char *data, std::size_t size) -> int
            {
                if (size == 0)
                    return 0;
                Platform::SocketTCP *tcp_socket = static_cast<Platform::SocketTCP *>(context);
                uint32_t write_feedback;
                if (!tcp_socket->Platform::SocketTCP::write_buffer(data, (uint32_t)size, &write_feedback))
                {
                    return MBEDTLS_ERR_NET_CONN_RESET;
                    // if (tcp_socket->Platform::SocketTCP::isClosed())
                    //     return MBEDTLS_ERR_NET_CONN_RESET;
                    // else
                    //     return MBEDTLS_ERR_SSL_WANT_WRITE;
                }
                return (int)write_feedback;
            },
            [](void *context, unsigned char *data, std::size_t size) -> int
            {
                if (size == 0)
                    return 0;
                Platform::SocketTCP *tcp_socket = static_cast<Platform::SocketTCP *>(context);
                uint32_t read_feedback;
                // tcp_socket->Platform::SocketTCP::setReadTimeout(0);
                if (!tcp_socket->Platform::SocketTCP::read_buffer(data, (uint32_t)size, &read_feedback))
                {
                    if (tcp_socket->Platform::SocketTCP::isReadTimedout())
                        return MBEDTLS_ERR_SSL_TIMEOUT;
                    return MBEDTLS_ERR_NET_CONN_RESET;
                    // if (tcp_socket->Platform::SocketTCP::isClosed())
                    //     return MBEDTLS_ERR_NET_CONN_RESET;
                    // else
                    //     return MBEDTLS_ERR_SSL_WANT_READ;
                }
                return (int)read_feedback;
            },
            nullptr
            // [](void *context, unsigned char *data, std::size_t size, uint32_t timeout_ms) -> int
            // {
            //     Platform::SocketTCP *tcp_socket = static_cast<Platform::SocketTCP *>(context);
            //     uint32_t read_feedback;
            //     tcp_socket->Platform::SocketTCP::setReadTimeout(timeout_ms);
            //     if (!tcp_socket->Platform::SocketTCP::read_buffer(data, (uint32_t)size, &read_feedback))
            //     {
            //         if (tcp_socket->Platform::SocketTCP::isReadTimedout())
            //             return MBEDTLS_ERR_SSL_TIMEOUT;
            //         else
            //             return MBEDTLS_ERR_NET_CONN_RESET;

            //         // if (tcp_socket->Platform::SocketTCP::isClosed())
            //         //     return MBEDTLS_ERR_NET_CONN_RESET;
            //         // else if (tcp_socket->Platform::SocketTCP::isReadTimedout())
            //         //     return MBEDTLS_ERR_SSL_TIMEOUT;
            //         // else
            //         //     return MBEDTLS_ERR_SSL_WANT_READ;
            //     }
            //     return (int)read_feedback;
            // }
        );
        return true;
    }

    bool SSLContext::doHandshake()
    {
        if (this->handshake_done)
            return true;
        if (this->certificate_chain == nullptr)
            return false;

        int result;
        bool should_retry;

        do
        {
            result = mbedtls_ssl_handshake(&ssl_context);
            should_retry = result != 0 &&
                           (result == MBEDTLS_ERR_SSL_WANT_READ ||
                            result == MBEDTLS_ERR_SSL_WANT_WRITE ||
                            result == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS ||
                            result == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS ||
                            // result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA ||
                            result == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET);
            // if (result == MBEDTLS_ERR_SSL_RECEIVED_EARLY_DATA)
            //     mbedtls_ssl_read_early_data(&ssl_context, ...);
            if (should_retry)
                Platform::Sleep::millis(1);
        } while (should_retry);

        if (result != 0)
        {
            printf("Error during TLS handshake: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());

            // print verification errors
            if (result == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
            {
                auto verifyResult = mbedtls_ssl_get_verify_result(&ssl_context);
                std::string errors;

#define X509_CRT_ERROR_INFO(error_code, error_code_str, human_readable_string) \
    if (verifyResult & error_code)                                             \
        errors += " - " human_readable_string "\n";
                MBEDTLS_X509_CRT_ERROR_INFO_LIST
#undef X509_CRT_ERROR_INFO

                if (!errors.empty())
                    errors.resize(errors.size() - 1);
                printf("TLS certificate verification failed:\n%s\n", errors.c_str());
            }

            release_structures();
            return false;
        }

        handshake_done = true;
        return true;
    }

    void SSLContext::close()
    {
        if (!this->handshake_done)
            return;

        int result = mbedtls_ssl_close_notify(&ssl_context);

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
        const char *cipersuiteName = mbedtls_ssl_get_ciphersuite(&ssl_context);
        if (cipersuiteName)
            return cipersuiteName;
        return "";
    }

    bool SSLContext::write_buffer(const uint8_t *data, uint32_t size, uint32_t *write_feedback)
    {
        *write_feedback = 0;
        if (!this->handshake_done)
            return false;

        int result;
        bool should_retry;

        uint32_t current_pos = 0;
        *write_feedback = current_pos;

        while (current_pos < size)
        {
            do
            {
                result = mbedtls_ssl_write(&ssl_context,
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
                if (should_retry)
                    Platform::Sleep::millis(1);
            } while (should_retry);

            if (result < 0)
            {
                printf("Error writing TLS data: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
                release_structures();
                return false;
            }
            else if (result == 0 || result == MBEDTLS_ERR_NET_CONN_RESET)
            {
                // connection was closed
                printf("TLS connection was closed during write.\n");
                release_structures();
                return false;
            }

            current_pos += (uint32_t)result;
            *write_feedback = current_pos;
        }

        return true;
    }

    bool SSLContext::read_buffer(uint8_t *data, uint32_t size, uint32_t *read_feedback)
    {
        *read_feedback = 0;

        if (!this->handshake_done)
            return false;

        int result;
        bool should_retry;

        do
        {
            result = mbedtls_ssl_read(&ssl_context, (unsigned char *)data, size);
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
            if (should_retry)
                Platform::Sleep::millis(1);
        } while (should_retry);

        if (result == 0 || result == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || result == MBEDTLS_ERR_NET_CONN_RESET)
        {
            printf("TLS connection was closed during read.\n");
            release_structures();
            return false;
        }
        else if (result < 0)
        {
            printf("Error reading TLS data: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
            release_structures();
            return false;
        }

        *read_feedback = (uint32_t)result;

        return true;
    }
}
