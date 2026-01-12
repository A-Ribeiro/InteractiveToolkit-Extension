#include <InteractiveToolkit-Extension/network/parser/HTTPWriter.h>
#include <InteractiveToolkit/AlgorithmCore/PatternMatch/array_index_of.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace ITKExtension
{
    namespace Network
    {
        static inline void *memcpy_custom(uint8_t *dst, const uint8_t *src, uint32_t size)
        {
            if (size == 0)
                return dst;
            return memcpy(dst, src, size);
            // // non overlapping buffers
            // if (src > dst + size || dst > src + size)
            //     return memcpy(dst, src, size);
            // // overlapping buffers, copy byte by byte
            // for (uint32_t i = 0; i < size; i++)
            //     dst[i] = src[i];
            // return dst;
        }

        HTTPWriter::HTTPWriter(uint32_t buffer_size) : unknown_size_data{}
        {
            this->buffer_size = buffer_size;
            this->writing_buffer = STL_Tools::make_unique<uint8_t[]>(buffer_size);
            this->writing_transfer_encoding_max_size = UINT32_MAX;
            this->state = HTTPWriterState::None;

            header_write_index = 0;
            header_count = 0;
            data_size = 0;
            body_total_size = 0;
            current_chunk_size = 0;
            unknown_size_data_size = 0;
        }

        void HTTPWriter::initialize(uint32_t writing_transfer_encoding_max_size,
                                    const HTTPWriterCallbacks &callbacks)
        {
            this->callbacks = callbacks;
            this->writing_transfer_encoding_max_size = writing_transfer_encoding_max_size;
        }

        bool HTTPWriter::start_streaming()
        {
            state = HTTPWriterState::WritingHeaders;
            header_write_index = 0;
            header_count = callbacks.getHeaderCount();
            return next();
        }

        bool HTTPWriter::next()
        {
            if (state == HTTPWriterState::Complete ||
                state == HTTPWriterState::Error)
                return false;

            if (state == HTTPWriterState::WritingHeaders)
            {
                if (header_write_index < header_count)
                {
                    if (!callbacks.getHeader(header_write_index, (char *)writing_buffer.get(), buffer_size, &data_size))
                    {
                        state = HTTPWriterState::Error;
                        return false;
                    }
                    header_write_index++;
                    return true;
                }
                else
                {
                    // transition to header CRLF
                    state = HTTPWriterState::WritingStartBodyWriteOnNextStep;
                    const char *crlf = "\r\n";
                    memcpy_custom(writing_buffer.get(), (const uint8_t *)crlf, 2);
                    data_size = 2;
                    return true;
                }
            }
            else if (state == HTTPWriterState::WritingStartBodyWriteOnNextStep)
            {
                bool is_body_chunked = false;
                int32_t size_from_stream = 0;
                if (!callbacks.startBodyStreaming(&is_body_chunked, &size_from_stream))
                {
                    state = HTTPWriterState::Error;
                    return false;
                }

                body_total_size = UINT_MAX;
                if (size_from_stream < 0)
                {
                    uint32_t can_read = (std::min)(buffer_size, writing_transfer_encoding_max_size);
                    if (!callbacks.getBodyPart(writing_buffer.get(), can_read, &data_size))
                    {
                        state = HTTPWriterState::Error;
                        return false;
                    }
                    if (data_size > can_read)
                    {
                        printf("[HTTPWriter] Body streaming returned more data than buffer size\n");
                        state = HTTPWriterState::Error;
                        return false;
                    }

                    if (data_size == 0)
                    {
                        const char *ending_size_str = "0\r\n";
                        memcpy_custom(writing_buffer.get(), (const uint8_t *)ending_size_str, 3);
                        data_size = 3;
                        state = HTTPWriterState::WritingBodyChunkedWriteLastCRLFOnNextStep;
                    }
                    else
                    {
                        unknown_size_data_size = snprintf((char *)unknown_size_data, sizeof(unknown_size_data), "%X\r\n", data_size);
                        state = HTTPWriterState::WritingBodyChunkedDataUnknownSize_ReturnSize;
                    }
                    return true;
                }
                else
                {
                    body_total_size = (uint32_t)size_from_stream;
                    if (is_body_chunked)
                    {
                        // transition to chunked body writing
                        current_chunk_size = (std::min)(body_total_size, writing_transfer_encoding_max_size);
                        body_total_size -= current_chunk_size;

                        // after this writing, writes crlf in the next step and finishes
                        if (current_chunk_size == 0)
                        {
                            const char *ending_size_str = "0\r\n";
                            memcpy_custom(writing_buffer.get(), (const uint8_t *)ending_size_str, 3);
                            data_size = 3;
                            state = HTTPWriterState::WritingBodyChunkedWriteLastCRLFOnNextStep;
                        }
                        else
                        {
                            data_size = (uint32_t)snprintf((char *)writing_buffer.get(), buffer_size, "%X\r\n", current_chunk_size);
                            state = HTTPWriterState::WritingBodyChunkedData;
                        }

                        return true;
                    }
                    else
                        state = HTTPWriterState::WritingBodyBinary;
                }
            }
            else if (state == HTTPWriterState::WritingBodyChunkedWriteLastCRLFOnNextStep)
            {
                const char *crlf = "\r\n";
                memcpy_custom(writing_buffer.get(), (const uint8_t *)crlf, 2);
                data_size = 2;
                state = HTTPWriterState::WritingBodyChunkedCompleteOnNextStep;
                return true;
            }
            else if (state == HTTPWriterState::WritingBodyChunkedCompleteOnNextStep)
            {
                state = HTTPWriterState::Complete;
                return false;
            }
            else if (state == HTTPWriterState::WritingBodyChunkedDataUnknownSize_ReturnSize)
            {
                state = HTTPWriterState::WritingBodyChunkedDataUnknownSize_ReturnData;
                return true;
            }
            else if (state == HTTPWriterState::WritingBodyChunkedDataUnknownSize_ReturnData)
            {
                // after return data... read more data
                uint32_t can_read = (std::min)(buffer_size, writing_transfer_encoding_max_size);
                if (!callbacks.getBodyPart(writing_buffer.get(), can_read, &data_size))
                {
                    state = HTTPWriterState::Error;
                    return false;
                }
                if (data_size > can_read)
                {
                    printf("[HTTPWriter] Body streaming returned more data than buffer size\n");
                    state = HTTPWriterState::Error;
                    return false;
                }
                if (data_size == 0)
                {
                    const char *ending_size_str = "0\r\n";
                    memcpy_custom(writing_buffer.get(), (const uint8_t *)ending_size_str, 3);
                    data_size = 3;
                    state = HTTPWriterState::WritingBodyChunkedWriteLastCRLFOnNextStep;
                }
                else
                {
                    unknown_size_data_size = snprintf((char *)unknown_size_data, sizeof(unknown_size_data), "%X\r\n", data_size);
                    state = HTTPWriterState::WritingBodyChunkedDataUnknownSize_ReturnSize;
                }
                return true;
            }
            else if (state == HTTPWriterState::WritingBodyChunkedDataCRLFOnNextStep)
            {
                const char *crlf = "\r\n";
                memcpy_custom(writing_buffer.get(), (const uint8_t *)crlf, 2);
                data_size = 2;
                state = HTTPWriterState::WritingBodyChunkedData;
                return true;
            }

            bool body_binary = (state == HTTPWriterState::WritingBodyBinary);
            bool body_chunked = (state == HTTPWriterState::WritingBodyChunkedData);

            if (body_binary || body_chunked)
            {

                if (body_binary && body_total_size == 0)
                {
                    state = HTTPWriterState::Complete;
                    return false;
                }

                if (body_chunked)
                {
                    if (current_chunk_size == 0)
                    {
                        // next chunk size
                        current_chunk_size = (std::min)(body_total_size, writing_transfer_encoding_max_size);
                        body_total_size -= current_chunk_size;

                        // after this writing, writes crlf in the next step and finishes
                        if (current_chunk_size == 0)
                        {
                            const char *ending_size_str = "0\r\n";
                            memcpy_custom(writing_buffer.get(), (const uint8_t *)ending_size_str, 3);
                            data_size = 3;
                            state = HTTPWriterState::WritingBodyChunkedWriteLastCRLFOnNextStep;
                        }
                        else
                            data_size = (uint32_t)snprintf((char *)writing_buffer.get(), buffer_size, "%X\r\n", current_chunk_size);

                        return true;
                    }
                }

                uint32_t can_read = body_binary ? (std::min)(body_total_size, buffer_size) : (std::min)(current_chunk_size, buffer_size);

                if (!callbacks.getBodyPart(writing_buffer.get(), can_read, &data_size))
                {
                    state = HTTPWriterState::Error;
                    return false;
                }
                if (data_size == 0)
                {
                    printf("[HTTPWriter] Body streaming ended before reaching total size\n");
                    state = HTTPWriterState::Error;
                    return false;
                }
                if (data_size > can_read)
                {
                    printf("[HTTPWriter] Body streaming returned more data than buffer size\n");
                    state = HTTPWriterState::Error;
                    return false;
                }

                if (body_binary)
                    body_total_size -= data_size;
                else
                {
                    current_chunk_size -= data_size;
                    if (current_chunk_size == 0)
                        state = HTTPWriterState::WritingBodyChunkedDataCRLFOnNextStep;
                }

                return true;
            }

            state = HTTPWriterState::Error;
            return false;
        }

        const uint8_t *HTTPWriter::get_data() const
        {
            if ((int)state < (int)HTTPWriterState::WritingHeaders ||
                (int)state > (int)HTTPWriterState::WritingBodyChunkedCompleteOnNextStep)
                return nullptr;
            if (state == HTTPWriterState::WritingBodyChunkedDataUnknownSize_ReturnSize)
                return unknown_size_data;
            return writing_buffer.get();
        }

        uint32_t HTTPWriter::get_data_size() const
        {
            if ((int)state < (int)HTTPWriterState::WritingHeaders ||
                (int)state > (int)HTTPWriterState::WritingBodyChunkedCompleteOnNextStep)
                return 0;
            if (state == HTTPWriterState::WritingBodyChunkedDataUnknownSize_ReturnSize)
                return unknown_size_data_size;
            return data_size;
        }

    }
}

#if defined(_WIN32)
#pragma warning(pop)
#endif