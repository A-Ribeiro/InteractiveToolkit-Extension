#pragma once

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#include "common.h"
#include <InteractiveToolkit/ITKCommon/FileSystem/File.h>

namespace ITKExtension
{
    namespace IO
    {
        class Reader
        {

            std::vector<uint8_t> buffer;
            size_t readPos;

            FILE *directStreamIn;

        public:
            Reader()
            {
                directStreamIn = nullptr;
            }

            void close()
            {
                buffer.resize(0);
            }

            void setDirectStreamIn(FILE *_file_descriptor)
            {
                directStreamIn = _file_descriptor;
                readPos = 0;
            }

            void readRaw(void *data, size_t size)
            {
                if (directStreamIn != nullptr)
                {
                    size_t readed = fread(data, sizeof(uint8_t), size, directStreamIn);

                    ITK_ABORT(readed != size, "Error to read from stream size request and readed size mismatch.");
                    return;
                }

                ITK_ABORT((readPos + size) > this->buffer.size(), "Error to read buffer. Size greater than the actual buffer is...");

                if (readPos >= buffer.size())
                {
                    memset(data, 0, size);
                    return;
                }
                memcpy(data, &buffer[readPos], size);
                readPos += size;

                if (readPos >= buffer.size())
                    buffer.resize(0);
            }

            bool readFromFile(const char *filename, bool compressed = true, std::string *errorStr = nullptr)
            {
                ON_COND_SET_ERRORSTR_RETURN(directStreamIn != nullptr, false, "directStreamIn is set.\n");

                if (!ITKCommon::FileSystem::File::FromPath(filename).readContentToVector(&buffer, errorStr))
                    return false;

                // buffer.resize(0);
                // FILE *in = ITKCommon::FileSystem::File::fopen(filename, "rb", errorStr);
                // if (!in)
                //     return false;

                // //if (in)
                // {
                //     //
                //     // optimized reading
                //     //
                //     fseek(in, 0, SEEK_END);
                //     buffer.resize(ftell(in));
                //     fseek(in, 0, SEEK_SET);
                //     int readed_size = (int)fread(buffer.data(), sizeof(uint8_t), buffer.size(), in);
                //     fclose(in);
                // }

                if (compressed)
                {
                    Platform::ObjectBuffer output_buffer;
                    if (!ITKWrappers::ZLIB::uncompress(
                            Platform::ObjectBuffer(buffer.data(), (int64_t)buffer.size()),
                            &output_buffer,
                            errorStr))
                        return false;

                    buffer.resize(output_buffer.size);
                    if (output_buffer.size > 0)
                        memcpy(buffer.data(), output_buffer.data, output_buffer.size);

                    // zlibWrapper::ZLIB zlib;
                    // zlib.uncompress(&buffer[0], (uint32_t)buffer.size());
                    // buffer = zlib.zlibOutput;
                }
                readPos = 0;

                return true;
            }

            bool readFromBuffer(const Platform::ObjectBuffer &objectBuffer, bool compressed = true, std::string *errorStr = nullptr)
            {
                ON_COND_SET_ERRORSTR_RETURN(directStreamIn != nullptr, false, "directStreamIn is set.\n");

                if (compressed)
                {

                    Platform::ObjectBuffer output_buffer;
                    if (!ITKWrappers::ZLIB::uncompress(
                            Platform::ObjectBuffer(objectBuffer.data, objectBuffer.size),
                            &output_buffer,
                            errorStr))
                        return false;

                    buffer.resize(output_buffer.size);
                    if (output_buffer.size > 0)
                        memcpy(buffer.data(), output_buffer.data, output_buffer.size);

                    // zlibWrapper::ZLIB zlib;
                    // zlib.uncompress(data, (uint32_t)size);
                    // buffer = zlib.zlibOutput;
                }
                else
                {
                    buffer.resize(objectBuffer.size);
                    memcpy(buffer.data(), objectBuffer.data, objectBuffer.size);
                }
                readPos = 0;

                return true;
            }

            uint8_t readUInt8()
            {
                uint8_t result;
                readRaw(&result, sizeof(uint8_t));
                return result;
            }

            int8_t readInt8()
            {
                int8_t result;
                readRaw(&result, sizeof(int8_t));
                return result;
            }

            uint16_t readUInt16()
            {
                uint16_t result;
                readRaw(&result, sizeof(uint16_t));
                return result;
            }

            int16_t readInt16()
            {
                int16_t result;
                readRaw(&result, sizeof(int16_t));
                return result;
            }

            uint32_t readUInt32()
            {
                uint32_t result;
                readRaw(&result, sizeof(uint32_t));
                return result;
            }

            int32_t readInt32()
            {
                int32_t result;
                readRaw(&result, sizeof(int32_t));
                return result;
            }

            uint64_t readUInt64()
            {
                uint64_t result;
                readRaw(&result, sizeof(uint64_t));
                return result;
            }

            int64_t readInt64()
            {
                int64_t result;
                readRaw(&result, sizeof(int64_t));
                return result;
            }

            float readFloat()
            {
                float result;
                readRaw(&result, sizeof(float));
                return result;
            }

            double readDouble()
            {
                double result;
                readRaw(&result, sizeof(double));
                return result;
            }

            bool readBool()
            {
                return readUInt8() != 0;
            }

            std::string readString()
            {
                std::string result;
                uint32_t size = readUInt32();
                if (size > 0)
                {
                    result.resize(size);
                    readRaw(&result[0], size);
                }
                return result;
            }

            void readBuffer(Platform::ObjectBuffer *buffer)
            {
                uint32_t size = readUInt32();
                buffer->setSize(size);
                if (size > 0)
                    readRaw(buffer->data, size);
            }
        };

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif
