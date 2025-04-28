#include "common/zeromq_client.hpp"
#include <iostream>
#include <sstream>

namespace trading {
namespace common {

ZeroMQClient::ZeroMQClient(const std::string& subscriber_endpoint, const std::string& publisher_endpoint)
    : context_(1), running_(false) {
    
    // Initialize publisher socket
    publisher_ = std::make_unique<zmq::socket_t>(context_, ZMQ_PUB);
    try {
        publisher_->connect(publisher_endpoint);
        std::cout << "Publisher connected to " << publisher_endpoint << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect publisher: " << e.what() << std::endl;
        throw;
    }

    // Initialize subscriber socket
    subscriber_ = std::make_unique<zmq::socket_t>(context_, ZMQ_SUB);
    try {
        subscriber_->connect(subscriber_endpoint);
        std::cout << "Subscriber connected to " << subscriber_endpoint << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect subscriber: " << e.what() << std::endl;
        throw;
    }

    // Allow time for connections to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

ZeroMQClient::~ZeroMQClient() {
    stop();
}

void ZeroMQClient::subscribe(const std::string& topic) {
    try {
        subscriber_->set(zmq::sockopt::subscribe, topic);
        std::cout << "Subscribed to topic: " << topic << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to subscribe to topic " << topic << ": " << e.what() << std::endl;
        throw;
    }
}

void ZeroMQClient::unsubscribe(const std::string& topic) {
    try {
        subscriber_->set(zmq::sockopt::unsubscribe, topic);
        std::cout << "Unsubscribed from topic: " << topic << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to unsubscribe from topic " << topic << ": " << e.what() << std::endl;
        throw;
    }
}

bool ZeroMQClient::publish(const std::string& topic, const std::string& message) {
    try {
        // Send topic
        zmq::message_t topic_msg(topic.data(), topic.size());
        auto topic_sent = publisher_->send(topic_msg, zmq::send_flags::sndmore);
        
        // Send message
        zmq::message_t data_msg(message.data(), message.size());
        auto data_sent = publisher_->send(data_msg, zmq::send_flags::none);
        
        return topic_sent && data_sent;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to publish message: " << e.what() << std::endl;
        return false;
    }
}

void ZeroMQClient::setMessageHandler(std::function<void(const std::string&, const std::string&)> handler) {
    message_handler_ = handler;
}

void ZeroMQClient::start() {
    if (running_) return;
    
    running_ = true;
    receive_thread_ = std::thread(&ZeroMQClient::receiveWorker, this);
    std::cout << "ZeroMQ client started" << std::endl;
}

void ZeroMQClient::stop() {
    if (!running_) return;
    
    running_ = false;
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    std::cout << "ZeroMQ client stopped" << std::endl;
}

void ZeroMQClient::receiveWorker() {
    while (running_) {
        try {
            zmq::message_t topic_msg;
            zmq::message_t data_msg;
            
            // Receive with timeout to allow checking running_ flag
            zmq::pollitem_t items[] = {
                { static_cast<void*>(*subscriber_), 0, ZMQ_POLLIN, 0 }
            };
            
            zmq::poll(items, 1, std::chrono::milliseconds(100));
            
            if (items[0].revents & ZMQ_POLLIN) {
                // Receive topic
                auto topic_recv = subscriber_->recv(topic_msg, zmq::recv_flags::none);
                if (!topic_recv) continue;
                
                // Receive message
                auto data_recv = subscriber_->recv(data_msg, zmq::recv_flags::none);
                if (!data_recv) continue;
                
                std::string topic(static_cast<char*>(topic_msg.data()), topic_msg.size());
                std::string message(static_cast<char*>(data_msg.data()), data_msg.size());
                
                if (message_handler_) {
                    message_handler_(topic, message);
                }
            }
        } catch (const zmq::error_t& e) {
            std::cerr << "Error in receive worker: " << e.what() << std::endl;
            // Small sleep to prevent CPU spinning on errors
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

} // namespace common
} // namespace trading
