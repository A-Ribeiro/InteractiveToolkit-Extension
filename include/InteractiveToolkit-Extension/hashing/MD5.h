#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace ITKExtension
{
    namespace Hashing
    {
        typedef uint8_t DigestArray16_T[16];

        // MD5 hashing
        class MD5
        {
        private:
            uint32_t state[4];
            uint64_t count;
            uint8_t buffer[64];
            void transform(const uint8_t block[64]);

        public:
            MD5();
            void reset();
            void update(const uint8_t *data, size_t len);
            void finalize(uint8_t digest[16]);

            // for convenience
            static void hash(const uint8_t *data, size_t len, uint8_t *digest_output);
            static void hash(const uint8_t *data, size_t len, uint8_t **digest_output);
            static void hash(const uint8_t *data, size_t len, DigestArray16_T *digest_output);
            static void hashFromFile(const char *filepath, uint8_t *digest_output, std::string *errorStr = nullptr);
            static void hashFromFile(const char *filepath, uint8_t **digest_output, std::string *errorStr = nullptr);
            static void hashFromFile(const char *filepath, DigestArray16_T *digest_output, std::string *errorStr = nullptr);
            static std::string hash(const uint8_t *data, size_t len);
            static std::string hash(const std::string &str);
            static std::string hash(const std::vector<uint8_t> &data);
            static std::string hashFromFile(const std::string &filepath, std::string *errorStr = nullptr);
        };
    }
}