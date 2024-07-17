#include <ITKWrappers/MD5.h>
#include <ITKWrappers/ZLIB.h>
#include <zlib.h>

namespace ITKWrappers
{
    namespace ZLIB
    {
        bool compress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            std::string *errorStr)
        {
            uLongf zlibOutput_Length = compressBound((uLong)input.size);
            output->setSize( INT64_C(16) + (int64_t)zlibOutput_Length + (int64_t)sizeof(uint32_t));

            uint32_t size_32_bits = (uint32_t)input.size;
            memcpy(&output->data[16], &size_32_bits, sizeof(uint32_t));

            int result = ::compress2((Bytef *)&output->data[16 + sizeof(uint32_t)],
                                     &zlibOutput_Length,
                                     (const Bytef *)input.data, (uLong)input.size, 
                                     Z_BEST_COMPRESSION);

            if (result != Z_OK){
                output->setSize(0);
                if (errorStr != nullptr)
                    *errorStr = ITKCommon::PrintfToStdString("Error to compress data");
                return false;
            }

            // after compression, the final size could be 
            // less than the limit bounds calculated
            output->setSize(INT64_C(16) + (int64_t)zlibOutput_Length + (int64_t)sizeof(uint32_t));

            MD5::get16bytesHashFromBytes(&output->data[16],
                                         (int64_t)zlibOutput_Length + (int64_t)sizeof(uint32_t),
                                         &output->data[0]);
            
            return true;
        }
        
        bool uncompress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            std::string *errorStr)
        {

            if (input.size < INT64_C(16) + (int64_t)sizeof(uint32_t)){
                output->setSize(0);
                if (errorStr != nullptr)
                    *errorStr = ITKCommon::PrintfToStdString("Error to uncompress stream");
                return false;
            }

            // Check the MD5 before create the uncompressed buffer
            uint8_t *md5_from_file = &input.data[0];
            uint8_t md5[16];
            MD5::get16bytesHashFromBytes(&input.data[16], (int64_t)(input.size - INT64_C(16)), md5);

            if (memcmp(md5_from_file, md5, 16) != 0){
                output->setSize(0);
                if (errorStr != nullptr)
                    *errorStr = ITKCommon::PrintfToStdString("Stream is corrupted");
                return false;
            }

            uLongf zlibUncompressed_Length = (uLongf)(*((uint32_t *)&input.data[16]));

            output->setSize((int64_t)zlibUncompressed_Length);
            int result = ::uncompress((Bytef *)&output->data[0],
                                      &zlibUncompressed_Length,
                                      (Bytef *)&input.data[16 + sizeof(uint32_t)],
                                      (uLong)(input.size - 16 - sizeof(uint32_t)));

            if (result != Z_OK || output->size != (int64_t)zlibUncompressed_Length){
                output->setSize(0);
                if (errorStr != nullptr)
                    *errorStr = ITKCommon::PrintfToStdString("Error to uncompress input stream");
                return false;
            }

            return true;
        }
    }
}
