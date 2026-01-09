#include <InteractiveToolkit-Extension/network/tls/GlobalSharedState.h>

#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#if (MBEDTLS_VERSION_MAJOR < 4)
#include <InteractiveToolkit/ITKCommon/Date.h>
#endif

#include <InteractiveToolkit-Extension/network/tls/TLSUtils.h>

#if (MBEDTLS_VERSION_MAJOR >= 4)
#include <psa/crypto.h>
#endif

namespace TLS
{

#if defined(MBEDTLS_THREADING_ALT)

    int GlobalSharedState::mutex_init(mbedtls_threading_mutex_t *ptr)
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
    void GlobalSharedState::mutex_destroy(mbedtls_threading_mutex_t *ptr)
    {
        delete static_cast<std::mutex *>(*ptr);
        *ptr = nullptr;
    }
    int GlobalSharedState::mutex_lock(mbedtls_threading_mutex_t *ptr)
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
    int GlobalSharedState::mutex_unlock(mbedtls_threading_mutex_t *ptr)
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

    int GlobalSharedState::cond_init(mbedtls_platform_condition_variable_t *ptr)
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
    void GlobalSharedState::cond_destroy(mbedtls_platform_condition_variable_t *ptr)
    {
        delete static_cast<std::condition_variable *>(*ptr);
        *ptr = nullptr;
    }
    int GlobalSharedState::cond_signal(mbedtls_platform_condition_variable_t *ptr)
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
    int GlobalSharedState::cond_broadcast(mbedtls_platform_condition_variable_t *ptr)
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
    int GlobalSharedState::cond_wait(mbedtls_platform_condition_variable_t *ptr,
                                     mbedtls_platform_mutex_t *mutex)
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

#endif

    GlobalSharedState::GlobalSharedState()
    {
#if defined(MBEDTLS_THREADING_ALT)
#if (MBEDTLS_VERSION_MAJOR < 4)
        mbedtls_threading_set_alt(
            &CustomGlobalThreading::mutex_init,
            &CustomGlobalThreading::mutex_destroy,
            &CustomGlobalThreading::mutex_lock,
            &CustomGlobalThreading::mutex_unlock);
#elif (MBEDTLS_VERSION_MAJOR >= 4)
        mbedtls_threading_set_alt(
            &GlobalSharedState::mutex_init,
            &GlobalSharedState::mutex_destroy,
            &GlobalSharedState::mutex_lock,
            &GlobalSharedState::mutex_unlock,

            &GlobalSharedState::cond_init,
            &GlobalSharedState::cond_destroy,
            &GlobalSharedState::cond_signal,
            &GlobalSharedState::cond_broadcast,
            &GlobalSharedState::cond_wait);
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
                                           (const unsigned char*) rng_curr_time_str.c_str(),
                                           (std::min)(rng_curr_time_str.length(), (size_t)MBEDTLS_CTR_DRBG_MAX_SEED_INPUT)
                                        );
        ITK_ABORT(result != 0, "Failed to seed DRBG: %s\n", TLSUtils::errorMessageFromReturnCode(result).c_str());
#else
        // crypto library init after or equal 4.x.x
        psa_status_t result = psa_crypto_init();
        ITK_ABORT(result != PSA_SUCCESS, "Failed to initialize crypto library\n");
#endif
    }

    GlobalSharedState::~GlobalSharedState()
    {
#if (MBEDTLS_VERSION_MAJOR < 4)
        mbedtls_ctr_drbg_free(&ctr_drbg_context);
        mbedtls_entropy_free(&entropy_context);
#endif

#if defined(MBEDTLS_THREADING_ALT)
        mbedtls_threading_free_alt();
#endif
    }

    GlobalSharedState *GlobalSharedState::Instance()
    {
        static GlobalSharedState instance;
        return &instance;
    }

    void GlobalSharedState::staticInitialization()
    {
        GlobalSharedState::Instance();
    }
}
