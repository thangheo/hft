trading/
├── include/
│   ├── common/
│   │   └── logger.hpp                    # Common logging interface
│   │
│   ├── data_ingestion/
│   │   ├── binance_connector.hpp         # Binance-specific exchange connector
│   │   ├── binance_parser.hpp            # Parser for Binance market data
│   │   ├── compression_config.hpp        # Configuration for compression libraries
│   │   ├── exchange_connector.hpp        # Base class for exchange connections
│   │   ├── exchange_manager.hpp          # Manages multiple exchange connections
│   │   ├── normalizer.hpp                # Standardizes exchange-specific data
│   │   ├── order_book.h                  # Alternative order book implementation
│   │   ├── order_book.hpp                # Order book data structures
│   │   ├── order_book_normalizer.hpp     # Normalizes order book data
│   │   ├── order_book_publisher.hpp      # ZeroMQ publisher for order book data
│   │   ├── publisher.hpp                 # Generic publisher interface
│   │   └── websocket_client.h            # WebSocket client for exchange connections
│   │
│   └── config/
│       └── features.hpp.in               # Feature configuration template
│
├── proto/
│   ├── order_book.proto                  # Protocol Buffer schema for order book data
│   ├── message_envelope.proto            # Wrapper message schema
│   └── heartbeat.proto                   # Heartbeat message schema
│
├── src/
│   ├── common/
│   │   └── logger.cpp                    # Logger implementation
│   │
│   └── data_ingestion/
│       ├── binance_connector.cpp         # Binance exchange connector implementation
│       ├── binance_parser.cpp            # Binance data parser implementation
│       ├── exchange_connector.cpp        # Base exchange connector implementation
│       ├── exchange_manager.cpp          # Exchange manager implementation
│       ├── normalizer.cpp                # Data normalizer implementation
│       ├── order_book.cpp                # Order book implementation
│       ├── order_book_normalizer.cpp     # Order book normalizer implementation
│       ├── order_book_publisher.cpp      # ZeroMQ publisher implementation
│       ├── publisher.cpp                 # Generic publisher implementation
│       └── websocket_client.cpp          # WebSocket client implementation
│
├── example/
│   ├── order_book_publisher_example.cpp  # Example using the publisher
│   └── order_book_subscriber_example.cpp # Example subscriber client
│
├── python/
│   └── clients/
│       └── order_book_subscriber.py      # Python client for order book data
│
└── tests/
    └── data_ingestion/
        ├── binance_parser_tests.cpp      # Tests for Binance parser
        ├── exchange_connector_tests.cpp  # Tests for exchange connectors
        ├── order_book_tests.cpp          # Tests for order book
        └── order_book_publisher_tests.cpp # Tests for ZeroMQ publisher