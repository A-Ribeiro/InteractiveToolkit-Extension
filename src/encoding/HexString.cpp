#include <InteractiveToolkit-Extension/encoding/HexString.h>

#include <string>
#include <vector>
#include <stdint.h>

namespace ITKExtension
{
    namespace Encoding
    {
        namespace HexString
        {
            // HexString encoding
            size_t EncodeComputeOutputSize(size_t len)
            {
                return len * 2;
            }

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

                const char hex_chars[] = "0123456789abcdef";
                for (size_t i = 0, j = 0; i < len; ++i, j += 2)
                {
                    outBuffer[j] = hex_chars[(data[i] >> 4) & 0x0F];
                    outBuffer[j + 1] = hex_chars[data[i] & 0x0F];
                }
                
                return true;
            }

            bool EncodeToString(const uint8_t *data, size_t len, std::string *outString)
            {
                size_t requiredSize = EncodeComputeOutputSize(len);
                outString->resize(requiredSize);
                return EncodeToBuffer(data, len, &(*outString)[0], outString->size());
            }

            bool EncodeToString(const std::vector<uint8_t> &data, std::string *outString) {
                return EncodeToString(data.data(), data.size(), outString);
            }

            // HexString decoding
            size_t DecodeComputeOutputSize(size_t len)
            {
                return len / 2;
            }

            bool DecodeToBuffer(const char *data, size_t len, uint8_t *outBuffer, size_t outBufferSize)
            {
                if (len % 2 != 0)
                    return false; // Invalid hex string length
                
                if (len == 0)
                {
                    if (outBufferSize != 0)
                        return false;
                    return true;
                }
                // check size
                size_t requiredSize = DecodeComputeOutputSize(len);
                if (outBufferSize != requiredSize)
                    // Handle error: output buffer has different size than required
                    return false;
                
                for (size_t i = 0; i < len; i += 2)
                {
                    char high = data[i];
                    char low = data[i + 1];
                    
                    uint8_t highVal = 0, lowVal = 0;
                    
                    if (high >= '0' && high <= '9')
                        highVal = high - '0';
                    else if (high >= 'a' && high <= 'f')
                        highVal = high - 'a' + 10;
                    else if (high >= 'A' && high <= 'F')
                        highVal = high - 'A' + 10;
                    else
                        return false; // Invalid character
                    
                    if (low >= '0' && low <= '9')
                        lowVal = low - '0';
                    else if (low >= 'a' && low <= 'f')
                        lowVal = low - 'a' + 10;
                    else if (low >= 'A' && low <= 'F')
                        lowVal = low - 'A' + 10;
                    else
                        return false; // Invalid character
                    
                    outBuffer[(i / 2)] = (highVal << 4) | lowVal;
                }

                return true;
            }

            bool DecodeToVector(const char *data, size_t len, std::vector<uint8_t> *outData)
            {
                size_t requiredSize = DecodeComputeOutputSize(len);
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