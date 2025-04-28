#include "data_ingestion/websocket_client.h"
#include <iostream>

namespace trading {
namespace data_ingestion {

WebSocketClient::WebSocketClient(const std::string& host, const std::string& port, 
                                const std::string& target, MessageCallback msgCallback,
                                ErrorCallback errCallback)
    : host_(host), port_(port), target_(target), 
      messageCallback_(std::move(msgCallback)), 
      errorCallback_(std::move(errCallback)),
      isConnected_(false) {
    
    // Keep io_context running
    work_ = std::make_unique<boost::asio::io_context::work>(ioc_);
    ioThread_ = std::make_unique<std::thread>([this]() { 
        try {
            ioc_.run(); 
        } catch(const std::exception& e) {
            std::cerr << "IO thread exception: " << e.what() << std::endl;
        }
    });
}

WebSocketClient::~WebSocketClient() {
    disconnect();
    
    // Stop io_context and join thread
    work_.reset();
    if (ioThread_ && ioThread_->joinable()) {
        ioThread_->join();
    }
}

void WebSocketClient::connect() {
    try {
        // Create the websocket stream
        ws_ = std::make_unique<boost::beast::websocket::stream<boost::beast::tcp_stream>>(ioc_);

        // Resolve the host
        boost::asio::ip::tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host_, port_);

        // Connect to the IP address and port
        auto ep = boost::beast::get_lowest_layer(*ws_).connect(results);

        // Set the host for the HTTP handshake
        ws_->set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type& req) {
                req.set(boost::beast::http::field::user_agent, "OrderImbalanceTrading/0.1");
            }));

        // Perform the websocket handshake
        ws_->handshake(host_, target_);
        isConnected_ = true;

        // Start reading
        doRead();
    } catch(const std::exception& e) {
        handleError("Connect error: " + std::string(e.what()));
    }
}

void WebSocketClient::disconnect() {
    if (isConnected_ && ws_) {
        try {
            ws_->close(boost::beast::websocket::close_code::normal);
            isConnected_ = false;
        } catch(const std::exception& e) {
            std::cerr << "Disconnect error: " << e.what() << std::endl;
        }
    }
}

void WebSocketClient::send(const std::string& message) {
    if (!isConnected_) {
        handleError("Cannot send: not connected");
        return;
    }

    boost::asio::post(ioc_, [this, message]() {
        try {
            ws_->write(boost::asio::buffer(message));
        } catch(const std::exception& e) {
            handleError("Send error: " + std::string(e.what()));
        }
    });
}

bool WebSocketClient::isConnected() const {
    return isConnected_;
}

void WebSocketClient::doRead() {
    if (!isConnected_) return;

    ws_->async_read(
        buffer_,
        [this](boost::system::error_code ec, std::size_t bytes_transferred) {
            onRead(ec, bytes_transferred);
        });
}

void WebSocketClient::onRead(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        isConnected_ = false;
        handleError("Read error: " + ec.message());
        return;
    }

    // Convert buffer to string
    std::string message = boost::beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    
    // Call the callback with the message
    if (messageCallback_) {
        messageCallback_(message);
    }

    // Queue another read
    doRead();
}

void WebSocketClient::handleError(const std::string& errorMsg) {
    if (errorCallback_) {
        errorCallback_(errorMsg);
    }
}

} // namespace data_ingestion
} // namespace trading
