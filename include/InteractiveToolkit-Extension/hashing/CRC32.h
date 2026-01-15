#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace ITKExtension
{
    namespace Hashing
    {
        typedef uint8_t DigestArray4_T[4];

        enum class CRC32Endianness
        {
            BigEndian,
            LittleEndian
        };
        // CRC32 hashing
        class CRC32
        {
        private:
            uint32_t state;
        public:
            CRC32();
            void reset();
            void update(const uint8_t *data, size_t len);
            void finalize(uint8_t digest[4], CRC32Endianness endianness = CRC32Endianness::LittleEndian);

            // for convenience
            static void hash(const uint8_t *data, size_t len, uint8_t *digest_output, CRC32Endianness endianness = CRC32Endianness::LittleEndian);
            static void hash(const uint8_t *data, size_t len, uint8_t **digest_output, CRC32Endianness endianness = CRC32Endianness::LittleEndian);
            static void hash(const uint8_t *data, size_t len, DigestArray4_T *digest_output, CRC32Endianness endianness = CRC32Endianness::LittleEndian);
            static void hashFromFile(const char *filepath, uint8_t *digest_output, CRC32Endianness endianness = CRC32Endianness::LittleEndian, std::string *errorStr = nullptr);
            static void hashFromFile(const char *filepath, uint8_t **digest_output, CRC32Endianness endianness = CRC32Endianness::LittleEndian, std::string *errorStr = nullptr);
            static void hashFromFile(const char *filepath, DigestArray4_T *digest_output, CRC32Endianness endianness = CRC32Endianness::LittleEndian, std::string *errorStr = nullptr);
            static std::string hash(const uint8_t *data, size_t len, CRC32Endianness endianness = CRC32Endianness::LittleEndian);
            static std::string hash(const std::string &str, CRC32Endianness endianness = CRC32Endianness::LittleEndian);
            static std::string hash(const std::vector<uint8_t> &data, CRC32Endianness endianness = CRC32Endianness::LittleEndian);
            static std::string hashFromFile(const std::string &filepath, CRC32Endianness endianness = CRC32Endianness::LittleEndian, std::string *errorStr = nullptr);
        };
    }
}