/**
 * @file binance_adapter.hpp
 * @brief Binance-specific message format conversion adapter
 * 
 * @details 
 * Part of the Data Ingestion & Processing Layer for the Crypto Trading System.
 * Converts Binance-specific order book message formats to the system's standard format.
 * 
 * Created on: April 30, 2025
 */

 #ifndef BINANCE_ADAPTER_HPP
 #define BINANCE_ADAPTER_HPP
 
 #include "data_ingestion/protocol/adapter_interface.hpp"
 #include "data_ingestion/types.hpp"
 
 /**
  * @class BinanceAdapter
  * @brief Adapter for processing Binance-specific message formats
  * 
  * Implements the AdapterInterface for Binance exchange messages,
  * handling conversion of Binance order book updates to the system's 
  * standardized format.
  */
 class BinanceAdapter : public AdapterInterface {
 public:
     /**
      * @brief Checks if the data is from Binance and can be handled by this adapter
      * 
      * @param data Raw exchange data received from websocket or REST API
      * @return bool True if the adapter can handle this data format
      */
     bool canHandle(const ExchangeData& data) override;
     
     /**
      * @brief Converts Binance-specific order book data to standard format
      * 
      * @param data Raw exchange data received from Binance
      * @return OrderBookUpdate Standardized order book update
      */
     OrderBookUpdate convertToStandardFormat(const ExchangeData& data) override;
 };
 
 #endif // BINANCE_ADAPTER_HPP