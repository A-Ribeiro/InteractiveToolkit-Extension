#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace ITKExtension
{
    namespace Hashing
    {
        typedef uint8_t DigestArray4_T[4];

        // CRC32 hashing
        class CRC32
        {
        private:
            uint32_t state;
        public:
            CRC32();
            void reset();
            void update(const uint8_t *data, size_t len);
            void finalize(uint8_t digest[4]);

            // for convenience
            static void hash(const uint8_t *data, size_t len, uint8_t *digest_output);
            static void hash(const uint8_t *data, size_t len, uint8_t **digest_output);
            static void hash(const uint8_t *data, size_t len, DigestArray4_T *digest_output);
            static void hashFromFile(const char *filepath, uint8_t *digest_output, std::string *errorStr = nullptr);
            static void hashFromFile(const char *filepath, uint8_t **digest_output, std::string *errorStr = nullptr);
            static void hashFromFile(const char *filepath, DigestArray4_T *digest_output, std::string *errorStr = nullptr);
            static std::string hash(const uint8_t *data, size_t len);
            static std::string hash(const std::string &str);
            static std::string hash(const std::vector<uint8_t> &data);
            static std::string hashFromFile(const std::string &filepath, std::string *errorStr = nullptr);
        };
    }
}