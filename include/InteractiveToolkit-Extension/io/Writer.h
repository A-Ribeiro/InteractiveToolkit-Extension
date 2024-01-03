#pragma once

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#include "common.h"

namespace ITKExtension
{
    namespace IO
    {
        class Writer
        {

            std::vector<uint8_t> buffer;
            FILE *directStreamOut;

        public:
            Writer()
            {
                directStreamOut = NULL;
            }

            void setDirectStreamOut(FILE *_file_descriptor)
            {
                directStreamOut = _file_descriptor;
            }

            void reset()
            {
                if (directStreamOut != NULL)
                    return;
                // buffer.clear();
                //  force deallocating writing buffer
                buffer.resize(0);
            }

            void writeRaw(const void *data, size_t size)
            {
                if (directStreamOut != NULL)
                {
                    size_t written = fwrite(data, sizeof(uint8_t), size, directStreamOut);
                    ITK_ABORT(written != size, "Error to write to stream write size and written size mismatch.");
                    return;
                }
                size_t startWrite = buffer.size();
                buffer.resize(startWrite + size);
                memcpy(&buffer[startWrite], data, size);
            }

            void writeToFile(const char *filename, bool compress = true)
            {
                if (directStreamOut != NULL)
                    return;

                if (compress)
                {
                    // zlibWrapper::ZLIB zlib;
                    // zlib.compress(&buffer[0],(uint32_t)buffer.size());
                    // buffer = zlib.zlibOutput;
                    Platform::ObjectBuffer output_buffer;
                    ITKWrappers::ZLIB::compress(
                        Platform::ObjectBuffer(buffer.data(), (uint32_t)buffer.size()),
                        &output_buffer,
                        [](const std::string &str_error)
                        {
                            // on Error
                            ITK_ABORT(true, str_error.c_str());
                        });

                    FILE *out = fopen(filename, "wb");
                    if (out != NULL)
                    {
                        if (output_buffer.size > 0)
                            fwrite(output_buffer.data, sizeof(uint8_t), output_buffer.size, out);
                        fclose(out);
                    }
                    reset();

                    return;
                }

                FILE *out = fopen(filename, "wb");
                if (out != NULL)
                {
                    if (buffer.size() > 0)
                        fwrite(buffer.data(), sizeof(uint8_t), buffer.size(), out);
                    fclose(out);
                }
                reset();
            }

            void writeToBuffer(Platform::ObjectBuffer *objectBuffer, bool compress = true)
            {
                if (directStreamOut != NULL)
                    return;

                if (compress)
                {
                    ITKWrappers::ZLIB::compress(
                        Platform::ObjectBuffer(buffer.data(), (uint32_t)buffer.size()),
                        objectBuffer,
                        [](const std::string &str_error)
                        {
                            // on Error
                            ITK_ABORT(true, str_error.c_str());
                        });
                    reset();
                    return;
                }

                objectBuffer->setSize((uint32_t)buffer.size());
                if (buffer.size() > 0)
                    memcpy(objectBuffer->data, buffer.data(), buffer.size());
                reset();
            }

            void writeUInt8(uint8_t v)
            {
                writeRaw(&v, sizeof(uint8_t));
            }

            void writeInt8(int8_t v)
            {
                writeRaw(&v, sizeof(int8_t));
            }

            void writeUInt16(uint16_t v)
            {
                writeRaw(&v, sizeof(uint16_t));
            }

            void writeInt16(int16_t v)
            {
                writeRaw(&v, sizeof(int16_t));
            }

            void writeUInt32(uint32_t v)
            {
                writeRaw(&v, sizeof(uint32_t));
            }

            void writeInt32(int32_t v)
            {
                writeRaw(&v, sizeof(int32_t));
            }

            void writeUInt64(const uint64_t &v)
            {
                writeRaw(&v, sizeof(uint64_t));
            }

            void writeInt64(const int64_t &v)
            {
                writeRaw(&v, sizeof(int64_t));
            }

            void writeFloat(float v)
            {
                writeRaw(&v, sizeof(float));
            }

            void writeDouble(const double &v)
            {
                writeRaw(&v, sizeof(double));
            }

            void writeBool(bool v)
            {
                if (v)
                    writeUInt8(255);
                else
                    writeUInt8(0);
            }

            void writeString(const std::string &s)
            {
                writeUInt32((uint32_t)s.size());
                if (s.size() > 0)
                    writeRaw(s.c_str(), s.size());
            }

            void writeBuffer(const Platform::ObjectBuffer &buffer)
            {
                writeUInt32(buffer.size);
                if (buffer.size > 0)
                    writeRaw(buffer.data, buffer.size);
            }
        };

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif