#pragma once

// #include "SocketTCP_SSL.h"
#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/EventCore/Callback.h>

namespace Platform
{
    class SocketTCP;
}

namespace ITKExtension
{
    namespace Network
    {

        struct HTTPWriterCallbacks
        {
            // header 0 is the first line
            EventCore::Callback<bool(int header_index, char *string_buffer_output, uint32_t string_buffer_max_bytes, uint32_t *str_length)> getHeader;

            // <0 error, 0 no more headers, >0 size of header written
            EventCore::Callback<int32_t()> getHeaderCount;

            // total_size = -1 forces chunked transfer encoding
            EventCore::Callback<bool(bool *is_body_chunked, int32_t *total_size)> startBodyStreaming;

            // if written is 0, no more body data
            EventCore::Callback<bool(uint8_t *data_output, uint32_t buffer_size, uint32_t *written)> getBodyPart;
        };

        enum class HTTPWriterState : int
        {
            None = 0,

            WritingHeaders,

            WritingStartBodyWriteOnNextStep,

            WritingBodyBinary,

            WritingBodyChunkedDataUnknownSize_ReturnSize,
            WritingBodyChunkedDataUnknownSize_ReturnData,

            WritingBodyChunkedData,
            WritingBodyChunkedDataCRLFOnNextStep,

            WritingBodyChunkedWriteLastCRLFOnNextStep,
            WritingBodyChunkedCompleteOnNextStep,

            Complete,
            Error
        };

        class HTTPWriter
        {
            // parameters
            HTTPWriterCallbacks callbacks;
            std::unique_ptr<uint8_t[]> writing_buffer;
            uint32_t buffer_size;

            // used for Transfer-Encoding: chunked
            uint32_t writing_transfer_encoding_max_size;

            // state
            int header_write_index;
            int header_count;
            uint32_t data_size;

            uint32_t body_total_size;
            uint32_t current_chunk_size;

            uint8_t unknown_size_data[16];
            uint32_t unknown_size_data_size;

        public:
            // delete assign operator and copy constructor
            HTTPWriter(const HTTPWriter &) = delete;
            HTTPWriter &operator=(const HTTPWriter &) = delete;

            HTTPWriterState state;

            // buffer_size needs to accomodate the max header size
            // or to be the same as parser_buffer_size to keep consistency
            HTTPWriter(uint32_t buffer_size);

            ~HTTPWriter() = default;

            void initialize(uint32_t writing_transfer_encoding_max_size,
                            const HTTPWriterCallbacks &callbacks);

            // resets the writing state machine,
            // even when is already in writing state or in error state
            bool start_streaming();

            // while get_data() != nullptr, next() must be called to advance the writing process
            const uint8_t *get_data() const;
            uint32_t get_data_size() const;

            bool next();
        };

    }
}