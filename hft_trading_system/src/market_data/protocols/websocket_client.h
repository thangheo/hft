// protocols/websocket_client.h

#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <set>

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    void setEndpoint(const std::string& endpoint);
    bool connect();
    void disconnect();

    void subscribe(const std::string& channel);
    void unsubscribe(const std::string& channel);

    void setMessageCallback(const std::function<void(const std::string&)>& callback);

private:
    void run();
    void handleMessage(const std::string& message);

    std::string endpoint_;
    std::function<void(const std::string&)> message_callback_;

    std::atomic<bool> is_connected_;
    std::thread ws_thread_;
    std::mutex subscription_mutex_;
    std::set<std::string> subscriptions_;

    // Add members for the actual WebSocket implementation (e.g., Boost.Asio)
};

#endif // WEBSOCKET_CLIENT_H
