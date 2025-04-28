#pragma once

#include <zmq.hpp>
#include <string>
#include <memory>
#include <thread>
#include <functional>
#include <atomic>
#include <chrono>
#include <queue>
#include <mutex>

namespace trading {
namespace common {

class ZeroMQClient {
public:
    ZeroMQClient(const std::string& subscriber_endpoint, const std::string& publisher_endpoint);
    virtual ~ZeroMQClient();

    // Subscribe to topics
    void subscribe(const std::string& topic);
    void unsubscribe(const std::string& topic);

    // Publish message
    bool publish(const std::string& topic, const std::string& message);

    // Start receiving messages
    void start();
    void stop();

    // Set callback for received messages
    void setMessageHandler(std::function<void(const std::string&, const std::string&)> handler);

protected:
    // ZeroMQ context and sockets
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> publisher_;
    std::unique_ptr<zmq::socket_t> subscriber_;

    // Message handling
    std::function<void(const std::string&, const std::string&)> message_handler_;
    std::thread receive_thread_;
    std::atomic<bool> running_;

    // Worker method for receive thread
    void receiveWorker();
};

} // namespace common
} // namespace trading
