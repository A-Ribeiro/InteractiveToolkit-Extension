#include <InteractiveToolkit-Extension/hashing/SHA256.h>
#include <InteractiveToolkit-Extension/encoding/HexString.h>
#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>

#include <string.h>

namespace ITKExtension
{
    namespace Hashing
    {
        static inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
        static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
        static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
        static inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
        static inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
        static inline uint32_t gamma0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
        static inline uint32_t gamma1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

        void SHA256::transform(const uint8_t block[64])
        {
            static const uint32_t k[64] = {
                0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
                0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
                0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
                0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
                0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
                0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
                0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

            uint32_t w[64];
            uint32_t a, b, c, d, e, f, g, h;

            // Prepare message schedule
            for (int i = 0; i < 16; ++i)
                w[i] = ((uint32_t)block[i * 4] << 24) |
                       ((uint32_t)block[i * 4 + 1] << 16) |
                       ((uint32_t)block[i * 4 + 2] << 8) |
                       ((uint32_t)block[i * 4 + 3]);

            for (int i = 16; i < 64; ++i)
                w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];

            // Initialize working variables
            a = state[0];
            b = state[1];
            c = state[2];
            d = state[3];
            e = state[4];
            f = state[5];
            g = state[6];
            h = state[7];

            // Main loop
            for (int i = 0; i < 64; ++i)
            {
                uint32_t t1 = h + sig1(e) + ch(e, f, g) + k[i] + w[i];
                uint32_t t2 = sig0(a) + maj(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
            }

            // Update state
            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
            state[5] += f;
            state[6] += g;
            state[7] += h;
        }

        SHA256::SHA256()
        {
            reset();
        }

        void SHA256::reset()
        {
            state[0] = 0x6a09e667;
            state[1] = 0xbb67ae85;
            state[2] = 0x3c6ef372;
            state[3] = 0xa54ff53a;
            state[4] = 0x510e527f;
            state[5] = 0x9b05688c;
            state[6] = 0x1f83d9ab;
            state[7] = 0x5be0cd19;
            count = 0;
            memset(buffer, 0, sizeof(buffer));
        }

        void SHA256::update(const uint8_t *data, size_t len)
        {
            size_t index = (count / 8) % 64;
            count += len * 8;

            size_t partLen = 64 - index;
            size_t i = 0;

            if (len >= partLen)
            {
                memcpy(&buffer[index], data, partLen);
                transform(buffer);

                for (i = partLen; i + 63 < len; i += 64)
                    transform(&data[i]);

                index = 0;
            }

            memcpy(&buffer[index], &data[i], len - i);
        }

        void SHA256::finalize(uint8_t digest[32])
        {
            uint8_t bits[8];
            uint64_t count_copy = count;

            // Store count in big-endian format
            for (int i = 7; i >= 0; --i)
            {
                bits[i] = count_copy & 0xff;
                count_copy >>= 8;
            }

            size_t index = (count / 8) % 64;
            size_t padLen = (index < 56) ? (56 - index) : (120 - index);

            uint8_t padding[64];
            memset(padding, 0, sizeof(padding));
            padding[0] = 0x80;

            update(padding, padLen);
            update(bits, 8);

            // Store state in big-endian format
            for (int i = 0; i < 8; ++i)
            {
                digest[i * 4] = (state[i] >> 24) & 0xff;
                digest[i * 4 + 1] = (state[i] >> 16) & 0xff;
                digest[i * 4 + 2] = (state[i] >> 8) & 0xff;
                digest[i * 4 + 3] = state[i] & 0xff;
            }
        }

        void SHA256::hash(const uint8_t *data, size_t len, uint8_t *digest_output)
        {
            SHA256 sha256;
            sha256.update(data, len);
            sha256.finalize(digest_output);
        }

        void SHA256::hash(const uint8_t *data, size_t len, uint8_t **digest_output)
        {
            hash(data, len, *digest_output);
        }

        void SHA256::hash(const uint8_t *data, size_t len, DigestArray32_T *digest_output)
        {
            hash(data, len, &(*digest_output)[0]);
        }

        void SHA256::hashFromFile(const char *filepath, uint8_t *digest_output, std::string *errorStr)
        {
            SHA256 sha256;
            FILE *file = ITKCommon::FileSystem::File::fopen(filepath, "rb", errorStr);
            if (!file)
            {
                memset(digest_output, 0, 16);
                return;
            }
            const int input_buff_size = 64 * 1024; // 64 KB
            unsigned char buffer[input_buff_size];
            size_t len;
            while ((len = fread(buffer, sizeof(unsigned char), input_buff_size, file)))
                sha256.update(buffer, len);
            ITKCommon::FileSystem::File::fclose(file, errorStr);
            sha256.finalize(digest_output);
        }

        void SHA256::hashFromFile(const char *file, uint8_t **digest_output, std::string *errorStr)
        {
            hashFromFile(file, *digest_output, errorStr);
        }

        void SHA256::hashFromFile(const char *file, DigestArray32_T *digest_output, std::string *errorStr)
        {
            hashFromFile(file, &(*digest_output)[0], errorStr);
        }

        std::string SHA256::hash(const uint8_t *data, size_t len)
        {
            uint8_t digest[32];
            hash(data, len, &digest);
            std::string output;
            Encoding::HexString::EncodeToString(digest, 32, &output);
            return output;
        }

        std::string SHA256::hash(const std::string &str)
        {
            return hash(reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
        }

        std::string SHA256::hash(const std::vector<uint8_t> &data)
        {
            return hash(data.data(), data.size());
        }

        std::string SHA256::hashFromFile(const std::string &filepath, std::string *errorStr)
        {
            uint8_t digest[32];
            hashFromFile(filepath.c_str(), &digest, errorStr);
            std::string output;
            Encoding::HexString::EncodeToString(digest, 32, &output);
            return output;
        }
    }
}