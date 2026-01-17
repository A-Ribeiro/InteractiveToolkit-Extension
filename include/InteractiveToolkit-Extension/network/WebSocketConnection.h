#pragma once

#include <InteractiveToolkit/common.h>
#include <InteractiveToolkit/EventCore/Callback.h>
#include <InteractiveToolkit/Platform/SocketTCP.h>
#include <InteractiveToolkit/Platform/Core/SmartVector.h>

#include "HTTPRequestAsync.h"
#include "HTTPResponseAsync.h"
#include "parser/HTTPParser.h"
#include "WebSocketFrame.h"

#include <memory>

namespace ITKExtension
{
    namespace Network
    {
        // Forward declarations
        class HTTPConnection;

        // WebSocket connection mode
        enum class WebSocketConnectionMode : int
        {
            Server, // Receives connection request
            Client  // Initiates connection request
        };

        // Main state machine for WebSocket connection
        enum class WebSocketConnectionState : int
        {
            // Handshake states
            Server_Handshaking,  // Server waiting for HTTP upgrade request
            Client_Handshaking,  // Client sending HTTP upgrade request and waiting for response

            // Connected state
            Connected,           // Handshake complete, ready to process frames

            // Processing state
            ProcessingConnection,// Actively processing frames

            // Terminal states
            Error,
            Closed
        };

        // Reading state machine
        enum class WebSocketReadingState : int
        {
            ReadingNone,
            ReadingWaitingFrame,
            ReadingFrameComplete,
            ReadingError
        };

        // Writing state machine
        enum class WebSocketWritingState : int
        {
            WritingNone,
            WritingFrame,
            WritingFrameComplete,
            WritingError
        };

        // Processing result for WebSocket operations
        enum class WebSocketProcessingResult : int
        {
            InProgress,
            Completed,
            Error
        };

        class WebSocketConnection
        {
        protected:
            // Socket configuration
            bool strategy_read_write_until_socket_would_block;

            uint32_t socket_buffer_size;
            uint32_t parser_buffer_size;
            uint32_t parser_max_header_count;
            uint32_t writer_buffer_size;
            uint32_t writing_transfer_encoding_max_size;
            uint64_t websocket_max_frame_size;

            // Buffers
            std::unique_ptr<uint8_t[]> socket_read_buffer;
            std::unique_ptr<uint8_t[]> socket_read_buffer_remaining;
            uint32_t socket_buffer_remaining_size;

            // Socket
            std::shared_ptr<Platform::SocketTCP> socket;

            // Mode (Server or Client)
            WebSocketConnectionMode mode;

            // State machines
            WebSocketConnectionState state;
            WebSocketReadingState reading_state;
            WebSocketWritingState writing_state;

            // HTTP objects for handshaking
            std::shared_ptr<HTTPRequestAsync> handshake_request;
            std::shared_ptr<HTTPResponseAsync> handshake_response;

            // HTTP Parser for handshake
            std::unique_ptr<HTTPParser> http_parser;

            // HTTP Writer for handshake
            std::unique_ptr<HTTPWriter> http_writer;
            uint32_t http_written_offset;

            // WebSocket frame handling
            std::unique_ptr<WebSocketFrameParser> frame_parser;
            std::unique_ptr<WebSocketFrameWriter> frame_writer;

            // Frame queues
            Platform::SmartVector<std::shared_ptr<WebSocketFrame>> incoming_frames;
            Platform::SmartVector<std::shared_ptr<WebSocketFrame>> outgoing_frames;

            // Current frame being written
            std::shared_ptr<WebSocketFrame> current_write_frame;
            uint32_t current_write_offset;

            // WebSocket handshake key (for validation)
            std::string websocket_key;
            std::string websocket_accept_key;

            // Mutex for writing operations : mutable to allow locking in const methods
            mutable Platform::Mutex writing_mutex;

            // Helper methods
            bool clientGenerateWebSocketKey();
            bool computeWebSocketAcceptKey(const std::string &key, std::string *accept_key);
            bool clientValidateWebSocketAcceptKey(const std::string &received_accept);

            // Handshake processing
            WebSocketProcessingResult processServerHandshake();
            WebSocketProcessingResult processClientHandshake();

            // Frame processing
            WebSocketProcessingResult processReading();
            WebSocketProcessingResult processWriting();

            // Handshake HTTP reading/writing
            WebSocketProcessingResult processHttpReading();
            WebSocketProcessingResult processHttpWriting();

            // Internal helpers
            bool sendHandshakeRequest(const std::string &host, const std::string &path);
            bool sendHandshakeResponse();
            bool parseUpgradeRequest();
            bool parseUpgradeResponse();

        public:
            WebSocketConnection(std::shared_ptr<Platform::SocketTCP> socket,
                                WebSocketConnectionMode mode,
                                bool strategy_read_write_until_socket_would_block = true,
                                uint32_t socket_buffer_size = 4 * 1024,
                                uint32_t parser_buffer_size = 4 * 1024,
                                uint32_t parser_max_header_count = 100,
                                uint32_t writer_buffer_size = 4 * 1024,
                                uint32_t writing_transfer_encoding_max_size = 16 * 1024,
                                uint64_t websocket_max_frame_size = 16 * 1024 * 1024);
            virtual ~WebSocketConnection();

            // Delete copy constructor and assignment operator
            WebSocketConnection(const WebSocketConnection &) = delete;
            WebSocketConnection &operator=(const WebSocketConnection &) = delete;

            // State accessors
            WebSocketConnectionState getState() const { return state; }
            WebSocketConnectionMode getMode() const { return mode; }
            WebSocketReadingState getReadingState() const { return reading_state; }
            WebSocketWritingState getWritingState() const { return writing_state; }

            std::shared_ptr<Platform::SocketTCP> &getSocket() { return socket; }

            // Connection lifecycle
            virtual void close();
            void closeWithCode(WebSocketCloseCode code = WebSocketCloseCode::Normal,
                               const std::string &reason = "");

            // Client: Initiate handshake
            bool clientBeginHandshake(const std::string &host, const std::string &path = "/",
                                      const std::vector<std::string> &protocols = {},
                                      const std::vector<std::pair<std::string, std::string>> &extra_headers = {});

            // Server: Get handshake request after headers are parsed
            std::shared_ptr<HTTPRequestAsync> getHandshakeRequest() const { return handshake_request; }

            // Get handshake response (for client validation)
            std::shared_ptr<HTTPResponseAsync> getHandshakeResponse() const { return handshake_response; }

            // Frame sending
            bool sendWritingFrame(std::shared_ptr<WebSocketFrame> frame);
            bool sendText(const std::string &text);
            bool sendBinary(const std::vector<uint8_t> &data);
            bool sendBinary(const uint8_t *data, size_t size);
            bool sendPing(const std::string &data = "");
            bool sendPong(const std::string &data = "");
            bool sendClose(WebSocketCloseCode code = WebSocketCloseCode::Normal,
                           const std::string &reason = "");

            // Frame receiving
            bool hasReadingFrame() const;
            std::shared_ptr<WebSocketFrame> getReadingFrame();

            // Check if there are pending outgoing frames
            bool hasOutgoingFrames() const;

            // Main processing method
            WebSocketProcessingResult process();
        };

    }
}
