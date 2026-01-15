#pragma once

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
            static inline constexpr size_t EncodeComputeOutputSize(size_t len) noexcept
            {
                return ((len + 2) / 3) * 4;
            }
            bool EncodeToBuffer(const uint8_t *data, size_t len, char *outBuffer, size_t outBufferSize);
            bool EncodeToString(const uint8_t *data, size_t len, std::string *outString);
            bool EncodeToString(const std::vector<uint8_t> &data, std::string *outString);
            
            // Base64 decoding
            static inline constexpr size_t DecodeComputeOutputSize(const char *data, size_t len) noexcept
            {
                return len < 2 ? 0 : (len / 4) * 3 - ((data[len - 1] == '=') ? 1 : 0) - ((data[len - 2] == '=') ? 1 : 0);
            }
            bool DecodeToBuffer(const char *data, size_t len, uint8_t *outBuffer, size_t outBufferSize);
            bool DecodeToVector(const char *data, size_t len, std::vector<uint8_t> *outData);
            bool DecodeToVector(const std::string &encoded, std::vector<uint8_t> *outData);
        }
    }
}