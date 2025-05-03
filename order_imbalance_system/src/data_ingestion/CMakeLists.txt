# Organize sources into groups
set(WEBSOCKET_SOURCES
    websocket/websocket_client_base.cpp
    websocket/connection_pool.cpp
    websocket/exchanges/binance_websocket_client.cpp
    # websocket/exchanges/binance_parser.cpp
    websocket/exchanges/binance_client_factory.cpp
    # websocket/exchanges/binance_parser.cpp
    # websocket/exchanges/coinbase_client.cpp
    websocket/exchanges/exchange_connector.cpp
    websocket/exchanges/exchange_manager.cpp
    # websocket/exchanges/ftx_client.cpp
)

set(REST_SOURCES
    rest/client_base.cpp
    rest/client_interface.cpp
    rest/exchanges/binance_client.cpp
    # rest/exchanges/coinbase_client.cpp
    # rest/exchanges/ftx_client.cpp
)

set(ORDERBOOK_SOURCES
    # orderbook/book.cpp
    # orderbook/level.cpp
    orderbook/order_book.cpp
    orderbook/reconstructor.cpp
)

set(PROTOCOL_SOURCES
    protocol/binance_adapter.cpp
    protocol/coinbase_adapter.cpp
    protocol/ftx_adapter.cpp
)

set(NORMALIZATION_SOURCES
    # normalization/normalizer.cpp
    normalization/order_book_normalizer.cpp
)

set(PUBLISHER_SOURCES
    # publisher/order_book_publisher.cpp
    publisher/zeromq_publisher.cpp
)

set(CACHE_SOURCES
    cache/order_book_cache.cpp
)

set(BLOCKCHAIN_SOURCES
    blockchain/data_collector.cpp
    blockchain/transaction_monitor.cpp
)

set(NEWS_SOURCES
    news/api_client.cpp
    news/processor.cpp
)

set(SENTIMENT_SOURCES
    sentiment/analyzer.cpp
    sentiment/collector.cpp
)

set(TIME_SOURCES
    time/synchronizer.cpp
)

set(QUALITY_SOURCES
    data_quality/filter.cpp
    data_quality/validator.cpp
)

set(STORAGE_SOURCES
    storage/influxdb_writer.cpp
)

set(MISC_SOURCES
    # connection_pool.cpp
    data_source_manager.cpp
    logging.cpp
)

# Combine all sources
set(DATA_INGESTION_SOURCES
    ${WEBSOCKET_SOURCES}
    ${REST_SOURCES}
    ${ORDERBOOK_SOURCES}
    ${PROTOCOL_SOURCES}
    ${NORMALIZATION_SOURCES}
    ${PUBLISHER_SOURCES}
    ${CACHE_SOURCES}
    ${BLOCKCHAIN_SOURCES}
    ${NEWS_SOURCES}
    ${SENTIMENT_SOURCES}
    ${TIME_SOURCES}
    ${QUALITY_SOURCES}
    ${STORAGE_SOURCES}
    ${MISC_SOURCES}
)

# Create the library
add_library(data_ingestion ${DATA_INGESTION_SOURCES})

target_link_libraries(data_ingestion
    PUBLIC
        common
        proto_lib
        Boost::system
        Boost::filesystem
        Boost::thread
        ZeroMQ::ZeroMQ
        cppzmq
        Threads::Threads
)

target_include_directories(data_ingestion
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Order book reconstructor standalone executable
add_executable(order_book_reconstructor examples/exchange_manager_example.cpp)
#  orderbook/order_book_reconstructor_main.cpp)
target_link_libraries(order_book_reconstructor
    PRIVATE
        data_ingestion
        common
)

# Install headers
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/data_ingestion/
    DESTINATION include/data_ingestion
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install the library and executable
install(TARGETS data_ingestion order_book_reconstructor
    EXPORT DataIngestionTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)