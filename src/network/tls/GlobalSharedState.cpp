#include <InteractiveToolkit-Extension/network/tls/PrivateKey.h>
#include <InteractiveToolkit-Extension/network/tls/SSLContext.h>
#include <InteractiveToolkit-Extension/network/tls/GlobalSharedState.h>

#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#include <mbedtls/version.h>
#include <mbedtls/threading.h>

#if (MBEDTLS_VERSION_MAJOR < 4)

#include <InteractiveToolkit/ITKCommon/Date.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

using mbedtls_thread_type_t = mbedtls_threading_mutex_t;

#elif (MBEDTLS_VERSION_MAJOR >= 4)

#include <psa/crypto.h>
using mbedtls_thread_type_t = mbedtls_platform_mutex_t;

#endif

#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>

namespace TLS
{

    // custom threading implementation
    class ThreadingMutexWrapper
    {

#if !defined(MBEDTLS_THREADING_C)
#error "Mbed TLS not built with threading support error (check your build flags)"
#elif defined(MBEDTLS_THREADING_ALT)

        static int mutex_init(mbedtls_thread_type_t *ptr)
        {
            try
            {
                *ptr = static_cast<void *>(new std::mutex());
            }
            catch (const std::exception &)
            {
                return MBEDTLS_ERR_THREADING_USAGE_ERROR;
            }
            return 0;
        }
        static void mutex_destroy(mbedtls_thread_type_t *ptr)
        {
            delete static_cast<std::mutex *>(*ptr);
            *ptr = nullptr;
        }
        static int mutex_lock(mbedtls_thread_type_t *ptr)
        {
            try
            {
                static_cast<std::mutex *>(*ptr)->lock();
            }
            catch (const std::exception &)
            {
                return MBEDTLS_ERR_THREADING_USAGE_ERROR;
            }
            return 0;
        }
        static int mutex_unlock(mbedtls_thread_type_t *ptr)
        {
            try
            {
                static_cast<std::mutex *>(*ptr)->unlock();
            }
            catch (const std::exception &)
            {
                return MBEDTLS_ERR_THREADING_USAGE_ERROR;
            }
            return 0;
        }

#if (MBEDTLS_VERSION_MAJOR >= 4)

        static int cond_init(mbedtls_platform_condition_variable_t *ptr)
        {
            try
            {
                *ptr = static_cast<void *>(new std::condition_variable());
            }
            catch (const std::exception &)
            {
                return MBEDTLS_ERR_THREADING_USAGE_ERROR;
            }
            return 0;
        }
        static void cond_destroy(mbedtls_platform_condition_variable_t *ptr)
        {
            delete static_cast<std::condition_variable *>(*ptr);
            *ptr = nullptr;
        }
        static int cond_signal(mbedtls_platform_condition_variable_t *ptr)
        {
            try
            {
                static_cast<std::condition_variable *>(*ptr)->notify_one();
            }
            catch (const std::exception &)
            {
                return MBEDTLS_ERR_THREADING_USAGE_ERROR;
            }
            return 0;
        }
        static int cond_broadcast(mbedtls_platform_condition_variable_t *ptr)
        {
            try
            {
                static_cast<std::condition_variable *>(*ptr)->notify_all();
            }
            catch (const std::exception &)
            {
                return MBEDTLS_ERR_THREADING_USAGE_ERROR;
            }
            return 0;
        }
        static int cond_wait(mbedtls_platform_condition_variable_t *ptr,
                             mbedtls_thread_type_t *mutex)
        {
            try
            {
                std::unique_lock<std::mutex> lock(*static_cast<std::mutex *>(*mutex), std::adopt_lock);
                static_cast<std::condition_variable *>(*ptr)->wait(lock);
                lock.release();
            }
            catch (const std::exception &)
            {
                return MBEDTLS_ERR_THREADING_USAGE_ERROR;
            }
            return 0;
        }
#endif

#elif !defined(MBEDTLS_THREADING_PTHREAD)
#error "Missing Mbed TLS threading implementation (check your build flags)"
#endif

        ThreadingMutexWrapper()
        {
#if defined(MBEDTLS_THREADING_ALT)
#if (MBEDTLS_VERSION_MAJOR < 4)
            mbedtls_threading_set_alt(
                &ThreadingMutexWrapper::mutex_init,
                &ThreadingMutexWrapper::mutex_destroy,
                &ThreadingMutexWrapper::mutex_lock,
                &ThreadingMutexWrapper::mutex_unlock);
#elif (MBEDTLS_VERSION_MAJOR >= 4)
            mbedtls_threading_set_alt(
                &ThreadingMutexWrapper::mutex_init,
                &ThreadingMutexWrapper::mutex_destroy,
                &ThreadingMutexWrapper::mutex_lock,
                &ThreadingMutexWrapper::mutex_unlock,

                &ThreadingMutexWrapper::cond_init,
                &ThreadingMutexWrapper::cond_destroy,
                &ThreadingMutexWrapper::cond_signal,
                &ThreadingMutexWrapper::cond_broadcast,
                &ThreadingMutexWrapper::cond_wait);
#endif
#endif

#if (MBEDTLS_VERSION_MAJOR < 4)
            // crypto library init before or equal 3.x.x

            std::string rng_curr_time_str = ITKCommon::PrintfToStdString(
                "%s-%s",
                "itkcommon-mbedtls-" MBEDTLS_VERSION_STRING_FULL,
                ITKCommon::Date::NowUTC().toISOString().c_str());

            mbedtls_entropy_init(&entropy_context);
            mbedtls_ctr_drbg_init(&ctr_drbg_context);

            int result = mbedtls_ctr_drbg_seed(&ctr_drbg_context,
                                               mbedtls_entropy_func,
                                               &entropy_context,
                                               (const unsigned char *)rng_curr_time_str.c_str(),
                                               (std::min)(rng_curr_time_str.length(), (size_t)MBEDTLS_CTR_DRBG_MAX_SEED_INPUT));
            ITK_ABORT(result != 0, "Failed to seed DRBG: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
#else
            // crypto library init after or equal 4.x.x
            psa_status_t result = psa_crypto_init();
            ITK_ABORT(result != PSA_SUCCESS, "Failed to initialize crypto library\n");
#endif
        }

    public:
#if (MBEDTLS_VERSION_MAJOR < 4)
        mbedtls_entropy_context entropy_context;
        mbedtls_ctr_drbg_context ctr_drbg_context;
#endif

        ~ThreadingMutexWrapper()
        {
#if (MBEDTLS_VERSION_MAJOR < 4)
            mbedtls_ctr_drbg_free(&ctr_drbg_context);
            mbedtls_entropy_free(&entropy_context);
#endif

#if defined(MBEDTLS_THREADING_ALT)
            mbedtls_threading_free_alt();
#endif
        }

        static ThreadingMutexWrapper *Instance()
        {
            static ThreadingMutexWrapper instance;
            return &instance;
        }
    };

    void GlobalSharedState::staticInitialization()
    {
        ThreadingMutexWrapper::Instance();
    }

    // in early versions, need to set RNG context for PrivateKey parsing
    int GlobalSharedState::setPrimaryKey(PrivateKey *key_instance,
                      const unsigned char *key_to_use, size_t key_to_use_length,
                      const unsigned char *key_decrypt_password, size_t key_decrypt_password_length)
    {
        int result;
#if MBEDTLS_VERSION_MAJOR == 3
        result = mbedtls_pk_parse_key(key_instance->private_key_context.get(),
                                      (const unsigned char *)key_to_use, key_to_use_length,
                                      (const unsigned char *)key_decrypt_password, key_decrypt_password_length,
                                      mbedtls_ctr_drbg_random,
                                      &ThreadingMutexWrapper::Instance()->ctr_drbg_context);
#else
        result = mbedtls_pk_parse_key(key_instance->private_key_context.get(),
                                      (const unsigned char *)key_to_use, key_to_use_length,
                                      (const unsigned char *)key_decrypt_password, key_decrypt_password_length);
#endif
        return result;
    }

    void GlobalSharedState::setSslRng(SSLContext *sslContext)
    {
        // random number generator explicitly is required before Mbed TLS 4.x.x
#if (MBEDTLS_VERSION_MAJOR < 4)
        mbedtls_ssl_conf_rng(sslContext->ssl_config.get(), mbedtls_ctr_drbg_random, &ThreadingMutexWrapper::Instance()->ctr_drbg_context);
#endif
    }

}
