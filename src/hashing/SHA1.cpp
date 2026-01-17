#include <InteractiveToolkit-Extension/hashing/SHA1.h>
#include <InteractiveToolkit-Extension/encoding/HexString.h>
#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>

#include <string.h>

namespace ITKExtension
{
    namespace Hashing
    {
        static inline uint32_t rotl(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

        void SHA1::transform(const uint8_t block[64])
        {
            static const uint32_t k[4] = {
                0x5A827999, // rounds 0-19
                0x6ED9EBA1, // rounds 20-39
                0x8F1BBCDC, // rounds 40-59
                0xCA62C1D6  // rounds 60-79
            };

            uint32_t w[80];
            uint32_t a, b, c, d, e;

            // Prepare message schedule
            for (int i = 0; i < 16; ++i)
                w[i] = ((uint32_t)block[i * 4] << 24) |
                       ((uint32_t)block[i * 4 + 1] << 16) |
                       ((uint32_t)block[i * 4 + 2] << 8) |
                       ((uint32_t)block[i * 4 + 3]);

            for (int i = 16; i < 80; ++i)
                w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

            // Initialize working variables
            a = state[0];
            b = state[1];
            c = state[2];
            d = state[3];
            e = state[4];

            // Main loop - 80 rounds
            for (int i = 0; i < 80; ++i)
            {
                uint32_t f, kval;

                if (i < 20)
                {
                    f = (b & c) | ((~b) & d);
                    kval = k[0];
                }
                else if (i < 40)
                {
                    f = b ^ c ^ d;
                    kval = k[1];
                }
                else if (i < 60)
                {
                    f = (b & c) | (b & d) | (c & d);
                    kval = k[2];
                }
                else
                {
                    f = b ^ c ^ d;
                    kval = k[3];
                }

                uint32_t temp = rotl(a, 5) + f + e + kval + w[i];
                e = d;
                d = c;
                c = rotl(b, 30);
                b = a;
                a = temp;
            }

            // Update state
            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
        }

        SHA1::SHA1()
        {
            reset();
        }

        void SHA1::reset()
        {
            state[0] = 0x67452301;
            state[1] = 0xEFCDAB89;
            state[2] = 0x98BADCFE;
            state[3] = 0x10325476;
            state[4] = 0xC3D2E1F0;
            count = 0;
            memset(buffer, 0, sizeof(buffer));
        }

        void SHA1::update(const uint8_t *data, size_t len)
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

        void SHA1::finalize(uint8_t digest[20])
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

            // Store state in big-endian format (5 * 4 = 20 bytes)
            for (int i = 0; i < 5; ++i)
            {
                digest[i * 4] = (state[i] >> 24) & 0xff;
                digest[i * 4 + 1] = (state[i] >> 16) & 0xff;
                digest[i * 4 + 2] = (state[i] >> 8) & 0xff;
                digest[i * 4 + 3] = state[i] & 0xff;
            }
        }

        void SHA1::hash(const uint8_t *data, size_t len, uint8_t *digest_output)
        {
            SHA1 SHA1;
            SHA1.update(data, len);
            SHA1.finalize(digest_output);
        }

        void SHA1::hash(const uint8_t *data, size_t len, uint8_t **digest_output)
        {
            hash(data, len, *digest_output);
        }

        void SHA1::hash(const uint8_t *data, size_t len, DigestArray20_T *digest_output)
        {
            hash(data, len, &(*digest_output)[0]);
        }

        void SHA1::hashFromFile(const char *filepath, uint8_t *digest_output, std::string *errorStr)
        {
            SHA1 SHA1;
            FILE *file = ITKCommon::FileSystem::File::fopen(filepath, "rb", errorStr);
            if (!file)
            {
                memset(digest_output, 0, 20);
                return;
            }
            const int input_buff_size = 64 * 1024; // 64 KB
            unsigned char buffer[input_buff_size];
            size_t len;
            while ((len = fread(buffer, sizeof(unsigned char), input_buff_size, file)))
                SHA1.update(buffer, len);
            ITKCommon::FileSystem::File::fclose(file, errorStr);
            SHA1.finalize(digest_output);
        }

        void SHA1::hashFromFile(const char *file, uint8_t **digest_output, std::string *errorStr)
        {
            hashFromFile(file, *digest_output, errorStr);
        }

        void SHA1::hashFromFile(const char *file, DigestArray20_T *digest_output, std::string *errorStr)
        {
            hashFromFile(file, &(*digest_output)[0], errorStr);
        }

        std::string SHA1::hash(const uint8_t *data, size_t len)
        {
            uint8_t digest[20];
            hash(data, len, &digest);
            std::string output;
            Encoding::HexString::EncodeToString(digest, 20, &output);
            return output;
        }

        std::string SHA1::hash(const std::string &str)
        {
            return hash(reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
        }

        std::string SHA1::hash(const std::vector<uint8_t> &data)
        {
            return hash(data.data(), data.size());
        }

        std::string SHA1::hashFromFile(const std::string &filepath, std::string *errorStr)
        {
            uint8_t digest[20];
            hashFromFile(filepath.c_str(), &digest, errorStr);
            std::string output;
            Encoding::HexString::EncodeToString(digest, 20, &output);
            return output;
        }
    }
}