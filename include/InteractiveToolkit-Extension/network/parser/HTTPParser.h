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

        struct HTTPParserCallbacks
        {
            EventCore::Callback<bool(const char *first_line)> onFirstLine;
            EventCore::Callback<bool(const char *key, const char *value)> onHeader;
            EventCore::Callback<bool(const uint8_t *remaining_data, uint32_t size)> onHeadersComplete;
            EventCore::Callback<bool(const uint8_t *data, uint32_t size)> onBodyPart;
            EventCore::Callback<bool()> onComplete;
        };

        enum class HTTPParserState : int
        {
            ReadingFirstLine = 0,
            ReadingHeaders,
            ReadingHeadersReady,
            ReadingHeadersReadyApplyNextBodyState,
            ReadingBodyUntilConnectionClose,
            ReadingBodyContentLength,
            ReadingBodyChunked,
            Complete,
            Error
        };

        class HTTPParser
        {
            // parameters
            HTTPParserCallbacks callbacks;
            std::unique_ptr<uint8_t[]> reading_buffer; // two buffers to improve memcpy performance

            uint8_t *input_buffer_a;    // two buffers to improve memcpy performance
            uint8_t *input_buffer_b;    // two buffers to improve memcpy performance
            uint32_t input_buffer_size; // input_buffer needs to be at least HTTP_MAX_HEADER_RAW_SIZE
            uint32_t max_header_size_bytes;
            uint32_t max_header_count;
            bool bytes_after_headers_are_body_data;

            // Header State

            // reading headers
            uint32_t input_buffer_start;
            uint32_t input_buffer_end;
            HTTPParserState body_state_after_headers;
            // bool first_line;

            // body special headers
            uint32_t content_length;
            bool is_chunked;

            bool content_length_present;
            bool transfer_encoding_present;

            // Chunk State
            enum ChunkedState
            {
                ReadChunkSize,
                ReadChunkData,
                ReadChunkCRLFAfterData
            };
            ChunkedState chunk_state;
            uint32_t chunk_expected_data_size;

            // ContentLength State
            uint32_t total_read;

            HTTPParserState _insertData(const uint8_t *data, uint32_t size, uint32_t *inserted_bytes);

        public:
            // delete assign operator and copy constructor
            HTTPParser(const HTTPParser &) = delete;
            HTTPParser &operator=(const HTTPParser &) = delete;

            HTTPParserState state;
            HTTPParser(uint32_t input_buffer_size,
                       uint32_t max_header_count);
            ~HTTPParser(); // calls connectionClosed()

            void initialize(bool bytes_after_headers_are_body_data,
                            const HTTPParserCallbacks &callbacks);

            HTTPParserState insertData(const uint8_t *data, uint32_t size, uint32_t *inserted_bytes);

            // mandatory only when bytes_after_headers_are_body_data is true
            void connectionClosed(bool show_log = true);
            void headersReadyApplyNextBodyState();

            // used in check state without any data...
            void checkOnlyStateTransition();
        };
        
    }
}