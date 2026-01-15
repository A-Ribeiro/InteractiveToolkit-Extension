#include <InteractiveToolkit-Extension/encoding/Base64.h>

#include <string>
#include <vector>
#include <stdint.h>

namespace ITKExtension
{
    namespace Encoding
    {
        namespace Base64
        {
            // Base64 encoding
            // size_t EncodeComputeOutputSize(size_t len)
            // {
            //     return ((len + 2) / 3) * 4;
            // }

            bool EncodeToBuffer(const uint8_t *data, size_t len, char *outBuffer, size_t outBufferSize)
            {
                if (len == 0)
                {
                    if (outBufferSize != 0)
                        return false;
                    return true;
                }
                // check size
                size_t requiredSize = EncodeComputeOutputSize(len);
                if (outBufferSize != requiredSize)
                    // Handle error: output buffer has different size than required
                    return false;

                static const char *base64_chars =
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

                for (size_t i = 0, j = 0; i < len; i += 3, j += 4)
                {
                    uint32_t val = data[i] << 16;
                    if (i + 1 < len)
                        val |= data[i + 1] << 8;
                    if (i + 2 < len)
                        val |= data[i + 2];

                    outBuffer[j] = base64_chars[(val >> 18) & 0x3F];
                    outBuffer[j + 1] = base64_chars[(val >> 12) & 0x3F];
                    outBuffer[j + 2] = (i + 1 < len) ? base64_chars[(val >> 6) & 0x3F] : '=';
                    outBuffer[j + 3] = (i + 2 < len) ? base64_chars[val & 0x3F] : '=';
                }

                return true;
            }

            bool EncodeToString(const uint8_t *data, size_t len, std::string *outString)
            {
                size_t requiredSize = EncodeComputeOutputSize(len);
                outString->resize(requiredSize);
                return EncodeToBuffer(data, len, &(*outString)[0], outString->size());
            }

            bool EncodeToString(const std::vector<uint8_t> &data, std::string *outString) 
            {
                return EncodeToString(data.data(),data.size(), outString);
            }

            // Base64 decoding
            // size_t DecodeComputeOutputSize(const char *data, size_t len)
            // {
            //     if (len < 2)
            //         return 0;
            //     size_t padding = 0;
            //     if (data[len - 1] == '=')
            //         padding++;
            //     if (data[len - 2] == '=')
            //         padding++;
            //     return (len / 4) * 3 - padding;
            // }

            bool DecodeToBuffer(const char *data, size_t len, uint8_t *outBuffer, size_t outBufferSize)
            {
                if (len == 0)
                {
                    if (outBufferSize != 0)
                        return false;
                    return true;
                }
                // check size
                size_t requiredSize = DecodeComputeOutputSize(data, len);
                if (outBufferSize != requiredSize)
                    // Handle error: output buffer has different size than required
                    return false;

                static const int8_t base64_table[256] = {
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
                    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
                    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
                    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

                size_t j = 0;
                for (size_t i = 0; i < len; i += 4)
                {
                    if (i + 3 >= len)
                        break;
                    int8_t a = base64_table[(uint8_t)data[i]];
                    int8_t b = base64_table[(uint8_t)data[i + 1]];
                    int8_t c = base64_table[(uint8_t)data[i + 2]];
                    int8_t d = base64_table[(uint8_t)data[i + 3]];
                    if (a == -1 || b == -1)
                        return false; // Invalid character
                    outBuffer[j++] = (uint8_t)((a << 2) | (b >> 4));
                    if (c != -1)
                    {
                        outBuffer[j++] = (uint8_t)((b << 4) | (c >> 2));
                        if (d != -1)
                            outBuffer[j++] = (uint8_t)((c << 6) | d);
                    }
                }
                return true;
            }

            bool DecodeToVector(const char *data, size_t len, std::vector<uint8_t> *outData)
            {
                size_t requiredSize = DecodeComputeOutputSize(data, len);
                outData->resize(requiredSize);
                return DecodeToBuffer(data, len, outData->data(), outData->size());
            }

            bool DecodeToVector(const std::string &encoded, std::vector<uint8_t> *outData)
            {
                return DecodeToVector(encoded.data(), encoded.length(), outData);
            }
        }
    }
}