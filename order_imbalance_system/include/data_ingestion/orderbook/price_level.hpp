// include/data_ingestion/orderbook/price_level.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace trading {
namespace data_ingestion {

/**
 * @class PriceLevel
 * @brief Represents a price level in an order book with associated quantity
 */
class PriceLevel {
public:
    // Default constructor
    PriceLevel() : price_(0.0), quantity_(0.0) {}
    
    // Constructor with double values
    PriceLevel(double price, double quantity) 
        : price_(price), quantity_(quantity) {}
    
    // Constructor with string values (with validation)
    PriceLevel(const std::string& price, const std::string& quantity) {
        try {
            price_ = std::stod(price);
            quantity_ = std::stod(quantity);
        } catch (const std::invalid_argument& e) {
            throw std::invalid_argument("Invalid price or quantity string format");
        } catch (const std::out_of_range& e) {
            throw std::out_of_range("Price or quantity value out of range");
        }
    }
    
    // Getters and setters
    double getPrice() const { return price_; }
    double getQuantity() const { return quantity_; }
    
    void setPrice(double price) { price_ = price; }
    void setQuantity(double quantity) { quantity_ = quantity; }
    
    // String conversion methods for external APIs
    std::string getPriceString() const { return std::to_string(price_); }
    std::string getQuantityString() const { return std::to_string(quantity_); }
    
    // Order management
    void addOrder(const std::string& order_id, double quantity) {
        orders_[order_id] = quantity;
        updateTotalQuantity();
    }
    
    void updateOrder(const std::string& order_id, double quantity) {
        if (orders_.find(order_id) != orders_.end()) {
            orders_[order_id] = quantity;
            updateTotalQuantity();
        }
    }
    
    void removeOrder(const std::string& order_id) {
        orders_.erase(order_id);
        updateTotalQuantity();
    }
    
    bool hasOrder(const std::string& order_id) const {
        return orders_.find(order_id) != orders_.end();
    }
    
    double getOrderQuantity(const std::string& order_id) const {
        auto it = orders_.find(order_id);
        if (it != orders_.end()) {
            return it->second;
        }
        return 0.0;
    }
    
    const std::unordered_map<std::string, double>& getOrders() const {
        return orders_;
    }
    
    size_t getOrderCount() const {
        return orders_.size();
    }
    
private:
    double price_;
    double quantity_;
    std::unordered_map<std::string, double> orders_; // order_id -> quantity
    
    void updateTotalQuantity() {
        quantity_ = 0.0;
        for (const auto& pair : orders_) {
            quantity_ += pair.second;
        }
    }
};

} // namespace data_ingestion
} // namespace trading