/**
 * @file level.hpp
 * @brief Definition of order book price level
 * 
 * @details 
 * Part of the Data Ingestion & Processing Layer for the Crypto Trading System.
 * 
 * Created on: April 30, 2025
 */
#ifndef DATA_INGESTION_ORDERBOOK_LEVEL_HPP
#define DATA_INGESTION_ORDERBOOK_LEVEL_HPP

#include <string>
#include <map>

namespace data_ingestion {
namespace orderbook {

// Represents a price level in an order book
class PriceLevel {
public:
    // Constructors
    PriceLevel(double price = 0.0, double quantity = 0.0);
    PriceLevel(const std::string& price_str, const std::string& quantity_str);
    
    // Getters
    double getPrice() const;
    double getQuantity() const;
    
    // Setters
    void setQuantity(double quantity);
    
    // Order management
    void addOrder(const std::string& order_id, double quantity);
    void updateOrder(const std::string& order_id, double quantity);
    void removeOrder(const std::string& order_id);
    const std::map<std::string, double>& getOrders() const;
    
    // Utility methods
    bool isEmpty() const;
    std::string toString() const;
    
private:
    double price_;
    double quantity_;
    std::map<std::string, double> orders_; // order_id -> quantity
    
    // Recalculate total quantity from individual orders
    void recalculateTotalQuantity();
};

} // namespace orderbook
} // namespace data_ingestion

#endif // DATA_INGESTION_ORDERBOOK_LEVEL_HPP