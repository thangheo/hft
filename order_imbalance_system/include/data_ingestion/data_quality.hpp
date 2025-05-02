// include/data_ingestion/data_quality.hpp
#pragma once

#include "data_ingestion/order_book_types.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace trading {
namespace data_ingestion {

// Base class for data quality filters
class DataQualityFilter {
public:
    virtual ~DataQualityFilter() = default;
    
    // Set next filter in chain
    void setNext(std::shared_ptr<DataQualityFilter> next) {
        next_ = next;
    }
    
    // Process the update through the filter chain
    bool process(OrderBookUpdate& update) {
        // Apply this filter
        if (!filter(update)) {
            return false;
        }
        
        // Apply next filter if it exists
        if (next_) {
            return next_->process(update);
        }
        
        return true;
    }
    
protected:
    // Implement this method in derived classes
    virtual bool filter(OrderBookUpdate& update) = 0;
    
    std::shared_ptr<DataQualityFilter> next_;
};

// Validates checksum
class ChecksumFilter : public DataQualityFilter {
protected:
    bool filter(OrderBookUpdate& update) override;
};

// Checks price ranges
class PriceRangeFilter : public DataQualityFilter {
public:
    PriceRangeFilter(const std::unordered_map<std::string, double>& max_prices = {});
    
protected:
    bool filter(OrderBookUpdate& update) override;
    
private:
    std::unordered_map<std::string, double> max_prices_;
    
    // Get reasonable price range for a symbol
    double getReasonablePriceRange(const std::string& symbol) const;
};

// Checks for sequence gaps
class SequenceGapFilter : public DataQualityFilter {
protected:
    bool filter(OrderBookUpdate& update) override;
    
private:
    std::unordered_map<std::string, int64_t> last_sequence_numbers_;
};

// Main quality control class
class DataQualityControl {
// include/data_ingestion/data_quality.hpp (continued)
public:
    DataQualityControl();
    
    // Add a custom filter
    void addFilter(std::shared_ptr<DataQualityFilter> filter);
    
    // Validate an update
    bool validateUpdate(OrderBookUpdate& update);
    
private:
    std::shared_ptr<DataQualityFilter> first_filter_;
    std::shared_ptr<DataQualityFilter> last_filter_;
};

} // namespace data_ingestion
} // namespace trading