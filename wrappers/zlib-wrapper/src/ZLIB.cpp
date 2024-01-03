#include <ITKWrappers/MD5.h>
#include <ITKWrappers/ZLIB.h>
#include <zlib.h>

namespace ITKWrappers
{
    namespace ZLIB
    {
        void compress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            EventCore::Callback<void(const std::string &)> onError)
        {
            uLongf zlibOutput_Length = compressBound(input.size);
            output->setSize(16 + (uint32_t)zlibOutput_Length + sizeof(uint32_t));

            memcpy(&output->data[16], &input.size, sizeof(uint32_t));

            int result = ::compress2((Bytef *)&output->data[16 + sizeof(uint32_t)],
                                     &zlibOutput_Length,
                                     (const Bytef *)input.data, input.size, 
                                     Z_BEST_COMPRESSION);

            if (result != Z_OK){
                output->setSize(0);
                if (onError != nullptr)
                    onError("Error to compress data");
                return;
            }

            // after compression, the final size could be 
            // less than the limit bounds calculated
            output->setSize(16 + (uint32_t)zlibOutput_Length + sizeof(uint32_t));

            MD5::get16bytesHashFromBytes((char *)&output->data[16],
                                         (int)((uint32_t)zlibOutput_Length + sizeof(uint32_t)),
                                         &output->data[0]);
        }
        
        void uncompress(
            const Platform::ObjectBuffer &input,
            Platform::ObjectBuffer *output,
            EventCore::Callback<void(std::string)> onError)
        {

            if (input.size < 16 + sizeof(uint32_t)){
                output->setSize(0);
                if (onError != nullptr)
                    onError("Error to decompress stream");
                return;
            }

            // Check the MD5 before create the uncompressed buffer
            unsigned char *md5_from_file = (unsigned char *)&input.data[0];
            unsigned char md5[16];
            MD5::get16bytesHashFromBytes((char *)&input.data[16], input.size - 16, md5);

            if (memcmp(md5_from_file, md5, 16) != 0){
                output->setSize(0);
                if (onError != nullptr)
                    onError("Stream is corrupted");
                return;
            }

            uLongf zlibUncompressed_Length = (uLongf)(*((uint32_t *)&input.data[16]));

            output->setSize((uint32_t)zlibUncompressed_Length);
            int result = ::uncompress((Bytef *)&output->data[0],
                                      &zlibUncompressed_Length,
                                      (Bytef *)&input.data[16 + sizeof(uint32_t)],
                                      input.size - 16 - sizeof(uint32_t));

            if (result != Z_OK || output->size != zlibUncompressed_Length){
                output->setSize(0);
                if (onError != nullptr)
                    onError("Error to uncompress input stream");
                return;
            }
        }
    }
}
