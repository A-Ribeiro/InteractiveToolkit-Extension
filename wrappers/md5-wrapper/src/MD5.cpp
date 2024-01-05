#include <InteractiveToolkit/ITKCommon/ITKCommon.h> // ITKAbort

#include <ITKWrappers/MD5.h>
#include <md5/md5.h>

// #include <fstream>
// #include <iostream>
// #include <string.h>
// #include <memory.h>
// #include <stdio.h>

namespace ITKWrappers
{
    namespace MD5
    {

        std::string getHexStringHashFromBytes(const char *buffer, int size)
        {
            unsigned char result[16];
            get16bytesHashFromBytes(buffer, size, result);
            return _16BytesToHexString(result);
        }

        void get16bytesHashFromBytes(const char *buffer, int size, unsigned char outBuffer[16])
        {
            md5_state_t state;
            md5_init(&state);
            md5_append(&state, (const md5_byte_t *)buffer, size);
            md5_finish(&state, (md5_byte_t *)outBuffer);
        }

        std::string getHexStringHashFromFile(const char *filename)
        {
            unsigned char result[16];
            get16bytesHashFromFile(filename, result);
            return _16BytesToHexString(result);
        }

        void get16bytesHashFromFile(const char *filename, unsigned char outBuffer[16])
        {
            md5_state_t state;
            md5_init(&state);
            FILE *file = fopen(filename, "rb");
            unsigned char buffer[1024];
            if (file != NULL)
            {
                size_t len;
                while ((len = fread(buffer, sizeof(unsigned char), 1024, file)))
                    md5_append(&state, (const md5_byte_t *)buffer, (int)len);
                fclose(file);
            }
            else
            {
                ITK_ABORT(true,"File not found: %s\n", filename);
            }
            md5_finish(&state, (md5_byte_t *)outBuffer);
        }

        std::string _16BytesToHexString(unsigned char md5[16])
        {
            char result[33];
            for (int i = 0; i < 16; i++)
                snprintf(&result[i * 2], 33, "%02x", md5[i]);
            return result;
        }
    }

}
