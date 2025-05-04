// include/data_ingestion/orderbook/price_level.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace trading {
namespace data_ingestion {
 
class PriceLevel {
    public:
        // Constructors
        PriceLevel() : price_(0.0), quantity_(0.0) {}
        PriceLevel(double price, double quantity) : price_(price), quantity_(quantity) {}
        
        // From string constructor if needed
        PriceLevel(const std::string& price, const std::string& quantity) {
            try {
                price_ = std::stod(price);
                quantity_ = std::stod(quantity);
            } catch (const std::exception& e) {
                throw std::invalid_argument("Invalid price or quantity format");
            }
        }
        
        // Basic getters and setters
        double getPrice() const { return price_; }
        double getQuantity() const { return quantity_; }
        
        void setPrice(double price) { price_ = price; }
        void setQuantity(double quantity) { quantity_ = quantity; }
        
    private:
        double price_;
        double quantity_;
        // No order tracking map
    };
} // namespace data_ingestion
} // namespace trading