// #include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort
#include <InteractiveToolkit/ITKCommon/ITKAbort.h>

#include <ITKWrappers/MD5.h>
#include <md5/md5.h>

#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>


// #include <fstream>
// #include <iostream>
// #include <string.h>
// #include <memory.h>
// #include <stdio.h>

#if defined(_WIN32)
#pragma warning( push )
#pragma warning( disable : 4996)
#endif

namespace ITKWrappers
{
    namespace MD5
    {

        std::string getHexStringHashFromBytes(const uint8_t *buffer, int64_t size)
        {
            uint8_t result[16];
            get16bytesHashFromBytes(buffer, size, result);
            return _16BytesToHexString(result);
        }

        void get16bytesHashFromBytes(const uint8_t *buffer, int64_t size, uint8_t outBuffer[16])
        {
            md5_state_t state;
            md5_init(&state);

            int64_t input_buff_size = 64*1024;// 64 KB
            int64_t offset = 0;
            while (offset < size){
                if (offset + input_buff_size > size){
                    input_buff_size = size - offset;
                }
                md5_append(&state, (const md5_byte_t *)&buffer[offset], input_buff_size);
                offset += input_buff_size;
            }
            
            md5_finish(&state, (md5_byte_t *)outBuffer);
        }

        std::string getHexStringHashFromFile(const char *filename, std::string *errorStr)
        {
            unsigned char result[16];
            if (!get16bytesHashFromFile(filename, result, errorStr))
                return "";
            return _16BytesToHexString(result);
        }

        bool get16bytesHashFromFile(const char *filename, uint8_t outBuffer[16], std::string *errorStr)
        {
            md5_state_t state;
            md5_init(&state);
            FILE *file = ITKCommon::FileSystem::File::fopen(filename, "rb", errorStr);
            if (!file)
                return false;
            unsigned char buffer[1024];
            size_t len;
            while ((len = fread(buffer, sizeof(unsigned char), 1024, file)))
                md5_append(&state, (const md5_byte_t *)buffer, (int)len);
            fclose(file);
            md5_finish(&state, (md5_byte_t *)outBuffer);
            return true;
        }

        std::string _16BytesToHexString(uint8_t md5[16])
        {
            char result[33];
            for (int i = 0; i < 16; i++)
                snprintf(&result[i * 2], 33, "%02x", md5[i]);
            return result;
        }
    }

}

#if defined(_WIN32)
#pragma warning( pop )
#endif
