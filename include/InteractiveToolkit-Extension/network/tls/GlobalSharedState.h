#pragma once

namespace TLS
{
    class PrivateKey;
    class SSLContext;

    class GlobalSharedState
    {
    private:
        // in early versions, need to set RNG context for PrivateKey parsing
        static int setPrimaryKey(PrivateKey *key_instance,
                          const unsigned char *key, size_t keylen,
                          const unsigned char *pwd, size_t pwdlen);
        // in early versions, need to set RNG context for SSLContext
        static void setSslRng(SSLContext *sslContext);
    public:
        static void staticInitialization();

        friend class TLS::PrivateKey;
        friend class TLS::SSLContext;
    };
}
