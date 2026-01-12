#pragma once

#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/EventCore/Callback.h>
#include <InteractiveToolkit/Platform/SocketTCP.h>

#include "HTTPRequestAsync.h"
#include "HTTPResponseAsync.h"
#include "parser/HTTPParser.h"

namespace ITKExtension
{
    namespace Network
    {

        enum class HTTPConnectionState : int
        {
            // Reading states
            Server_ReadingRequest,
            Server_ReadingRequestComplete,

            // Writing states
            Server_WritingResponse,
            Server_WritingResponseComplete,

            // Reading states
            Client_WritingRequest,
            Client_WritingRequestComplete,

            // Writing states
            Client_ReadingResponse,
            Client_ReadingResponseComplete,

            // Error and close states
            Error,
            Closed
        };

        enum class HTTPConnectionMode : int
        {
            Server, // Reads request, writes response
            Client  // Writes request, reads response
        };

        enum class HTTPProcessingResult : int
        {
            InProgress,
            Completed,
            Error
        };

        class HTTPConnection
        {
        private:

            bool strategy_read_write_until_socket_would_block;

            uint32_t socket_buffer_size;
            uint32_t parser_buffer_size;
            uint32_t parser_max_header_count;
            uint32_t writer_buffer_size;
            uint32_t writing_transfer_encoding_max_size;

            std::unique_ptr<uint8_t[]> socket_read_buffer;
            
            // Socket and buffers
            std::shared_ptr<Platform::SocketTCP> socket;

            // Server or Client mode
            HTTPConnectionMode mode;

            // State management
            HTTPConnectionState state;

            // HTTP data
            std::shared_ptr<HTTPRequestAsync> request;
            std::shared_ptr<HTTPResponseAsync> response;

            // HTTP Parser for reading
            // server reads request, client reads response
            std::unique_ptr<HTTPParser> parser;

            // HTTP Writer for writing
            // server writes response, client writes request
            std::unique_ptr<HTTPWriter> writer;
            
            // Helper methods
            HTTPProcessingResult processReading();
            HTTPProcessingResult processWriting();

            uint32_t written_offset;

            static bool validateBeforeWrite(std::shared_ptr<HTTPBaseAsync> http_obj, uint32_t _writing_transfer_encoding_max_size);

        public:
            HTTPConnection(std::shared_ptr<Platform::SocketTCP> socket,
                           HTTPConnectionMode mode,
                           bool strategy_read_write_until_socket_would_block = true,
                           uint32_t socket_buffer_size = 4 * 1024,
                           uint32_t parser_buffer_size = 4 * 1024,
                           uint32_t parser_max_header_count = 100,
                           uint32_t writer_buffer_size = 4 * 1024,
                           uint32_t writing_transfer_encoding_max_size = 16 * 1024);
            ~HTTPConnection();

            // Delete copy constructor and assignment operator
            HTTPConnection(const HTTPConnection &) = delete;
            HTTPConnection &operator=(const HTTPConnection &) = delete;

            // State management
            HTTPConnectionState getState() const { return state; }
            HTTPConnectionMode getMode() const { return mode; }

            std::shared_ptr<Platform::SocketTCP> &getSocket() { return socket; }

            void close();

            // Data access
            std::shared_ptr<HTTPRequestAsync> getRequest() const { return request; }
            std::shared_ptr<HTTPResponseAsync> getResponse() const { return response; }

            // For server mode: call this after processing request to prepare response
            bool serverBeginWriteResponse(std::shared_ptr<HTTPResponseAsync> response);

            // For client mode: call this to send a request
            bool clientBeginWriteRequest(std::shared_ptr<HTTPRequestAsync> request);
            bool clientBeginReadResponse();

            // Main processing methods
            HTTPProcessingResult process();
        };

    }
}
