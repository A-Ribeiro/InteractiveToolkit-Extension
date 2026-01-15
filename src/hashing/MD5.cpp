#include <InteractiveToolkit-Extension/hashing/MD5.h>
#include <InteractiveToolkit-Extension/encoding/HexString.h>
#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>

#include <string.h>

namespace ITKExtension
{
    namespace Hashing
    {
        static inline uint32_t rotateLeft(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }
        static inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
        static inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
        static inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
        static inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

        void MD5::transform(const uint8_t block[64])
        {
            uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
            uint32_t x[16];

            for (int i = 0; i < 16; ++i)
                x[i] = (uint32_t)block[i * 4] |
                       ((uint32_t)block[i * 4 + 1] << 8) |
                       ((uint32_t)block[i * 4 + 2] << 16) |
                       ((uint32_t)block[i * 4 + 3] << 24);

            // Round 1
            a = b + rotateLeft(a + F(b, c, d) + x[0] + 0xd76aa478, 7);
            d = a + rotateLeft(d + F(a, b, c) + x[1] + 0xe8c7b756, 12);
            c = d + rotateLeft(c + F(d, a, b) + x[2] + 0x242070db, 17);
            b = c + rotateLeft(b + F(c, d, a) + x[3] + 0xc1bdceee, 22);
            a = b + rotateLeft(a + F(b, c, d) + x[4] + 0xf57c0faf, 7);
            d = a + rotateLeft(d + F(a, b, c) + x[5] + 0x4787c62a, 12);
            c = d + rotateLeft(c + F(d, a, b) + x[6] + 0xa8304613, 17);
            b = c + rotateLeft(b + F(c, d, a) + x[7] + 0xfd469501, 22);
            a = b + rotateLeft(a + F(b, c, d) + x[8] + 0x698098d8, 7);
            d = a + rotateLeft(d + F(a, b, c) + x[9] + 0x8b44f7af, 12);
            c = d + rotateLeft(c + F(d, a, b) + x[10] + 0xffff5bb1, 17);
            b = c + rotateLeft(b + F(c, d, a) + x[11] + 0x895cd7be, 22);
            a = b + rotateLeft(a + F(b, c, d) + x[12] + 0x6b901122, 7);
            d = a + rotateLeft(d + F(a, b, c) + x[13] + 0xfd987193, 12);
            c = d + rotateLeft(c + F(d, a, b) + x[14] + 0xa679438e, 17);
            b = c + rotateLeft(b + F(c, d, a) + x[15] + 0x49b40821, 22);

            // Round 2
            a = b + rotateLeft(a + G(b, c, d) + x[1] + 0xf61e2562, 5);
            d = a + rotateLeft(d + G(a, b, c) + x[6] + 0xc040b340, 9);
            c = d + rotateLeft(c + G(d, a, b) + x[11] + 0x265e5a51, 14);
            b = c + rotateLeft(b + G(c, d, a) + x[0] + 0xe9b6c7aa, 20);
            a = b + rotateLeft(a + G(b, c, d) + x[5] + 0xd62f105d, 5);
            d = a + rotateLeft(d + G(a, b, c) + x[10] + 0x02441453, 9);
            c = d + rotateLeft(c + G(d, a, b) + x[15] + 0xd8a1e681, 14);
            b = c + rotateLeft(b + G(c, d, a) + x[4] + 0xe7d3fbc8, 20);
            a = b + rotateLeft(a + G(b, c, d) + x[9] + 0x21e1cde6, 5);
            d = a + rotateLeft(d + G(a, b, c) + x[14] + 0xc33707d6, 9);
            c = d + rotateLeft(c + G(d, a, b) + x[3] + 0xf4d50d87, 14);
            b = c + rotateLeft(b + G(c, d, a) + x[8] + 0x455a14ed, 20);
            a = b + rotateLeft(a + G(b, c, d) + x[13] + 0xa9e3e905, 5);
            d = a + rotateLeft(d + G(a, b, c) + x[2] + 0xfcefa3f8, 9);
            c = d + rotateLeft(c + G(d, a, b) + x[7] + 0x676f02d9, 14);
            b = c + rotateLeft(b + G(c, d, a) + x[12] + 0x8d2a4c8a, 20);

            // Round 3
            a = b + rotateLeft(a + H(b, c, d) + x[5] + 0xfffa3942, 4);
            d = a + rotateLeft(d + H(a, b, c) + x[8] + 0x8771f681, 11);
            c = d + rotateLeft(c + H(d, a, b) + x[11] + 0x6d9d6122, 16);
            b = c + rotateLeft(b + H(c, d, a) + x[14] + 0xfde5380c, 23);
            a = b + rotateLeft(a + H(b, c, d) + x[1] + 0xa4beea44, 4);
            d = a + rotateLeft(d + H(a, b, c) + x[4] + 0x4bdecfa9, 11);
            c = d + rotateLeft(c + H(d, a, b) + x[7] + 0xf6bb4b60, 16);
            b = c + rotateLeft(b + H(c, d, a) + x[10] + 0xbebfbc70, 23);
            a = b + rotateLeft(a + H(b, c, d) + x[13] + 0x289b7ec6, 4);
            d = a + rotateLeft(d + H(a, b, c) + x[0] + 0xeaa127fa, 11);
            c = d + rotateLeft(c + H(d, a, b) + x[3] + 0xd4ef3085, 16);
            b = c + rotateLeft(b + H(c, d, a) + x[6] + 0x04881d05, 23);
            a = b + rotateLeft(a + H(b, c, d) + x[9] + 0xd9d4d039, 4);
            d = a + rotateLeft(d + H(a, b, c) + x[12] + 0xe6db99e5, 11);
            c = d + rotateLeft(c + H(d, a, b) + x[15] + 0x1fa27cf8, 16);
            b = c + rotateLeft(b + H(c, d, a) + x[2] + 0xc4ac5665, 23);

            // Round 4
            a = b + rotateLeft(a + I(b, c, d) + x[0] + 0xf4292244, 6);
            d = a + rotateLeft(d + I(a, b, c) + x[7] + 0x432aff97, 10);
            c = d + rotateLeft(c + I(d, a, b) + x[14] + 0xab9423a7, 15);
            b = c + rotateLeft(b + I(c, d, a) + x[5] + 0xfc93a039, 21);
            a = b + rotateLeft(a + I(b, c, d) + x[12] + 0x655b59c3, 6);
            d = a + rotateLeft(d + I(a, b, c) + x[3] + 0x8f0ccc92, 10);
            c = d + rotateLeft(c + I(d, a, b) + x[10] + 0xffeff47d, 15);
            b = c + rotateLeft(b + I(c, d, a) + x[1] + 0x85845dd1, 21);
            a = b + rotateLeft(a + I(b, c, d) + x[8] + 0x6fa87e4f, 6);
            d = a + rotateLeft(d + I(a, b, c) + x[15] + 0xfe2ce6e0, 10);
            c = d + rotateLeft(c + I(d, a, b) + x[6] + 0xa3014314, 15);
            b = c + rotateLeft(b + I(c, d, a) + x[13] + 0x4e0811a1, 21);
            a = b + rotateLeft(a + I(b, c, d) + x[4] + 0xf7537e82, 6);
            d = a + rotateLeft(d + I(a, b, c) + x[11] + 0xbd3af235, 10);
            c = d + rotateLeft(c + I(d, a, b) + x[2] + 0x2ad7d2bb, 15);
            b = c + rotateLeft(b + I(c, d, a) + x[9] + 0xeb86d391, 21);

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
        }

        MD5::MD5()
        {
            reset();
        }

        void MD5::reset()
        {
            state[0] = 0x67452301;
            state[1] = 0xefcdab89;
            state[2] = 0x98badcfe;
            state[3] = 0x10325476;
            count = 0;
            memset(buffer, 0, sizeof(buffer));
        }

        void MD5::update(const uint8_t *data, size_t len)
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

        void MD5::finalize(uint8_t digest[16])
        {
            uint8_t bits[8];
            uint64_t count_copy = count;

            for (int i = 0; i < 8; ++i)
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

            for (int i = 0; i < 4; ++i)
            {
                digest[i * 4] = state[i] & 0xff;
                digest[i * 4 + 1] = (state[i] >> 8) & 0xff;
                digest[i * 4 + 2] = (state[i] >> 16) & 0xff;
                digest[i * 4 + 3] = (state[i] >> 24) & 0xff;
            }
        }

        void MD5::hash(const uint8_t *data, size_t len, uint8_t *digest_output)
        {
            MD5 md5;
            md5.update(data, len);
            md5.finalize(digest_output);
        }

        void MD5::hash(const uint8_t *data, size_t len, uint8_t **digest_output)
        {
            hash(data, len, *digest_output);
        }

        void MD5::hash(const uint8_t *data, size_t len, DigestArray16_T *digest_output)
        {
            hash(data, len, &(*digest_output)[0]);
        }

        void MD5::hashFromFile(const char *filepath, uint8_t *digest_output, std::string *errorStr)
        {
            MD5 md5;
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
                md5.update(buffer, len);
            ITKCommon::FileSystem::File::fclose(file, errorStr);
            md5.finalize(digest_output);
        }

        void MD5::hashFromFile(const char *file, uint8_t **digest_output, std::string *errorStr)
        {
            hashFromFile(file, *digest_output, errorStr);
        }

        void MD5::hashFromFile(const char *file, DigestArray16_T *digest_output, std::string *errorStr)
        {
            hashFromFile(file, &(*digest_output)[0], errorStr);
        }

        std::string MD5::hash(const uint8_t *data, size_t len)
        {
            uint8_t digest[16];
            hash(data, len, &digest);
            std::string output;
            Encoding::HexString::EncodeToString(digest, 16, &output);
            return output;
        }

        std::string MD5::hash(const std::string &str)
        {
            return hash(reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
        }

        std::string MD5::hash(const std::vector<uint8_t> &data)
        {
            return hash(data.data(), data.size());
        }

        std::string MD5::hashFromFile(const std::string &filepath, std::string *errorStr)
        {
            uint8_t digest[16];
            hashFromFile(filepath.c_str(), &digest, errorStr);
            std::string output;
            Encoding::HexString::EncodeToString(digest, 16, &output);
            return output;
        }

    }
}