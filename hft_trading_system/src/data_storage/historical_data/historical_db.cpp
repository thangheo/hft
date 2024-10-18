// historical_db.cpp

#include "historical_db.h"
#include "src/data_storage/serialization/serializer.h"
#include <iostream>
#include <pqxx/pqxx>
#include <memory>

HistoricalDB::HistoricalDB() : conn_(nullptr) {}

HistoricalDB::~HistoricalDB() {
    disconnect();
}

bool HistoricalDB::connect(const std::string& connection_string) {
    try {
        conn_ = std::make_unique<pqxx::connection>(connection_string);
        if (conn_->is_open()) {
            return true;
        } else {
            std::cerr << "Failed to open database connection." << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        return false;
    }
}

void HistoricalDB::disconnect() {
    if (conn_) {
        // conn_->disconnect();
        conn_.reset();  // Reset the smart pointer
    }
}

bool HistoricalDB::storeMarketData(const MarketData& data) {
    if (!conn_) {
        std::cerr << "No database connection available." << std::endl;
        return false;
    }

    try {
        pqxx::work txn(*conn_);
        std::string bids_json =     /* Convert bids to JSON string */;
        std::string asks_json = /* Convert asks to JSON string */;

        txn.exec_params(
            "INSERT INTO market_data (exchange, symbol, timestamp, bids, asks) VALUES ($1, $2, $3, $4::jsonb, $5::jsonb)",
            data.exchange,
            data.symbol,
            std::chrono::system_clock::to_time_t(data.timestamp),
            bids_json,
            asks_json
        );
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Database insert error: " << e.what() << std::endl;
        return false;
    }
}

bool HistoricalDB::storeOrder(const Order& order) {
    if (!conn_) {
        std::cerr << "No database connection available." << std::endl;
        return false;
    }

    try {
        pqxx::work txn(*conn_);

        txn.exec_params(
            "INSERT INTO orders (order_id, client_order_id, symbol, side, price, quantity, status, timestamp) VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
            order.order_id,
            order.client_order_id,
            order.symbol,
            order.side,
            order.price,
            order.quantity,
            to_string(order.status),
            std::chrono::system_clock::to_time_t(order.timestamp)
        );
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Database insert error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<MarketData> HistoricalDB::queryMarketData(
    const std::string& symbol,
    const std::chrono::system_clock::time_point& start_time,
    const std::chrono::system_clock::time_point& end_time) {

    std::vector<MarketData> results;
    if (!conn_) {
        std::cerr << "No database connection available." << std::endl;
        return results;
    }

    try {
        pqxx::work txn(*conn_);

        std::string query = R"(
            SELECT exchange, symbol, timestamp, bids, asks
            FROM market_data
            WHERE symbol = $1
            AND timestamp BETWEEN $2 AND $3
            ORDER BY timestamp ASC
        )";

        pqxx::result res = txn.exec_params(
            query,
            symbol,
            std::chrono::system_clock::to_time_t(start_time),
            std::chrono::system_clock::to_time_t(end_time)
        );

        for (const auto& row : res) {
            MarketData data;
            data.exchange = row["exchange"].c_str();
            data.symbol = row["symbol"].c_str();
            data.timestamp = std::chrono::system_clock::from_time_t(row["timestamp"].as<std::time_t>());
            // Deserialize bids and asks from JSONB
            // Implement JSON parsing (e.g., using nlohmann::json or another JSON library)
            results.push_back(data);
        }

        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Database query error: " << e.what() << std::endl;
    }

    return results;
}
