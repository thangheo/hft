/**
 * @file level.cpp
 * @brief Implementation of order book price level
 * 
 * @details 
 * Part of the Data Ingestion & Processing Layer for the Crypto Trading System.
 * 
 * Created on: April 30, 2025
 */

 #include "data_ingestion/orderbook/level.hpp"
 #include <sstream>
 #include <iomanip>
 #include <algorithm>
 
 namespace data_ingestion {
 namespace orderbook {
 
 PriceLevel::PriceLevel(double price, double quantity)
     : price_(price), quantity_(quantity) {
 }
 
 PriceLevel::PriceLevel(const std::string& price_str, const std::string& quantity_str) {
     try {
         price_ = std::stod(price_str);
         quantity_ = std::stod(quantity_str);
     } catch (const std::exception& e) {
         // Handle conversion errors
         price_ = 0.0;
         quantity_ = 0.0;
     }
 }
 
 double PriceLevel::getPrice() const {
     return price_;
 }
 
 double PriceLevel::getQuantity() const {
     return quantity_;
 }
 
 void PriceLevel::setQuantity(double quantity) {
     quantity_ = quantity;
 }
 
 void PriceLevel::addOrder(const std::string& order_id, double quantity) {
     orders_[order_id] = quantity;
     recalculateTotalQuantity();
 }
 
 void PriceLevel::updateOrder(const std::string& order_id, double quantity) {
     if (orders_.find(order_id) != orders_.end()) {
         orders_[order_id] = quantity;
         recalculateTotalQuantity();
     }
 }
 
 void PriceLevel::removeOrder(const std::string& order_id) {
     orders_.erase(order_id);
     recalculateTotalQuantity();
 }
 
 const std::map<std::string, double>& PriceLevel::getOrders() const {
     return orders_;
 }
 
 bool PriceLevel::isEmpty() const {
     return quantity_ <= 0.0 || orders_.empty();
 }
 
 std::string PriceLevel::toString() const {
     std::stringstream ss;
     ss << std::fixed << std::setprecision(8) << price_ << ":" 
        << std::fixed << std::setprecision(8) << quantity_;
     return ss.str();
 }
 
 void PriceLevel::recalculateTotalQuantity() {
     quantity_ = 0.0;
     for (const auto& [order_id, qty] : orders_) {
         quantity_ += qty;
     }
 }
 
 } // namespace orderbook
 } // namespace data_ingestion