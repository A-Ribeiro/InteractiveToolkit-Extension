#pragma once

#include <mbedtls/version.h>

#include <mbedtls/threading.h>

#if (MBEDTLS_VERSION_MAJOR < 4)
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#else
// #include <mbedtls/threading.h>
// #include <psa/crypto_config.h>
#endif

// #include <mbedtls/error.h>
// #include <mbedtls/net_sockets.h>
// #include <mbedtls/ssl.h>

namespace TLS
{

#if (MBEDTLS_VERSION_MAJOR >= 4)
    using mbedtls_threading_mutex_t = mbedtls_platform_mutex_t;
#endif

    // custom threading implementation
    class GlobalSharedState
    {

#if !defined(MBEDTLS_THREADING_C)
#error "Mbed TLS not built with threading support error (check your build flags)"
#elif defined(MBEDTLS_THREADING_ALT)

        static int mutex_init(mbedtls_threading_mutex_t *ptr);
        static void mutex_destroy(mbedtls_threading_mutex_t *ptr);
        static int mutex_lock(mbedtls_threading_mutex_t *ptr);
        static int mutex_unlock(mbedtls_threading_mutex_t *ptr);

#if (MBEDTLS_VERSION_MAJOR >= 4)

        static int cond_init(mbedtls_platform_condition_variable_t *ptr);
        static void cond_destroy(mbedtls_platform_condition_variable_t *ptr);
        static int cond_signal(mbedtls_platform_condition_variable_t *ptr);
        static int cond_broadcast(mbedtls_platform_condition_variable_t *ptr);
        static int cond_wait(mbedtls_platform_condition_variable_t *ptr,
                             mbedtls_platform_mutex_t *mutex);
#endif

#elif !defined(MBEDTLS_THREADING_PTHREAD)
#error "Missing Mbed TLS threading implementation (check your build flags)"
#endif

        GlobalSharedState();

    public:
#if (MBEDTLS_VERSION_MAJOR < 4)
        mbedtls_entropy_context entropy_context;
        mbedtls_ctr_drbg_context ctr_drbg_context;
#endif

        ~GlobalSharedState();

        static GlobalSharedState *Instance();
        static void staticInitialization();
    };
}
