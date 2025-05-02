// quality_control.cpp

DataQualityControl::DataQualityControl() {
    // Set up standard filter chain
    auto checksumFilter = std::make_shared<ChecksumFilter>();
    auto priceRangeFilter = std::make_shared<PriceRangeFilter>();
    auto sequenceGapFilter = std::make_shared<SequenceGapFilter>();
    
    // Chain them together
    firstFilter = checksumFilter;
    checksumFilter->setNext(priceRangeFilter);
    priceRangeFilter->setNext(sequenceGapFilter);
    lastFilter = sequenceGapFilter;
}

void DataQualityControl::addFilter(std::shared_ptr<DataQualityFilter> filter) {
    if (lastFilter) {
        lastFilter->setNext(filter);
        lastFilter = filter;
    } else {
        firstFilter = lastFilter = filter;
    }
}

bool DataQualityControl::validateUpdate(OrderBookUpdate& update) {
    if (!firstFilter) return true;
    return firstFilter->process(update);
}

// Specific filter implementations
bool ChecksumFilter::filter(OrderBookUpdate& update) {
    // Skip if no checksum provided
    if (update.checksum.empty()) return true;
    
    // Validate checksum against reconstructed order book
    // This might require access to the OrderBookReconstructor
    // or could be done differently based on exchange
    return true; // Simplified
}

bool PriceRangeFilter::filter(OrderBookUpdate& update) {
    double reasonableRange = getReasonablePriceRange(update.symbol);
    
    // Check if any price is outside reasonable range
    for (const auto& level : update.bids) {
        if (level.price <= 0 || level.price > reasonableRange) {
            return false;
        }
    }
    
    for (const auto& level : update.asks) {
        if (level.price <= 0 || level.price > reasonableRange) {
            return false;
        }
    }
    
    return true;
}

double PriceRangeFilter::getReasonablePriceRange(const std::string& symbol) const {
    // This would be based on recent price history and volatility
    // Simplified implementation
    if (symbol == "BTCUSDT") return 100000.0;
    if (symbol == "ETHUSDT") return 10000.0;
    return 1000000.0; // Default fallback
}

bool SequenceGapFilter::filter(OrderBookUpdate& update) {
    auto key = update.exchangeId + ":" + update.symbol;
    
    auto it = lastSequenceNumbers.find(key);
    if (it == lastSequenceNumbers.end()) {
        // First update for this symbol
        lastSequenceNumbers[key] = update.sequenceNumber;
        return true;
    }
    
    // Check for sequence gap
    if (update.sequenceNumber <= it->second) {
        // Out of order or duplicate
        return false;
    }
    
    if (update.sequenceNumber > it->second + 1 && !update.isSnapshot) {
        // Gap detected - may need to request a snapshot
        // For now, we'll accept it but this could trigger a recovery process
        // Log warning
    }
    
    lastSequenceNumbers[key] = update.sequenceNumber;
    return true;
}