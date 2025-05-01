set(EXECUTION_SOURCES
    # Core components
    api_client_manager.cpp
    authentication_service.cpp
    execution_algorithm_factory.cpp
    execution_service.cpp
    exchange_api_client.cpp
    fill_reconciliation.cpp
    order_book.cpp
    order_generator.cpp
    order_lifecycle_tracker.cpp
    order_type_selector.cpp
    rate_limit_controller.cpp
    request_prioritizer.cpp
    smart_order_router.cpp
    
    # Exchanges
    exchanges/binance_api_client.cpp
    exchanges/ftx_api_client.cpp
    
    # Algorithms
    algorithms/twap_algorithm.cpp
    algorithms/vwap_algorithm.cpp
    algorithms/iceberg_algorithm.cpp
    
    # Models
    models/order.cpp
    models/execution_report.cpp
    
    # gRPC services
    grpc/execution_service_impl.cpp
)

add_library(execution ${EXECUTION_SOURCES})

target_link_libraries(execution
    PUBLIC
        common
        data_ingestion
        proto_lib
        Boost::system
        Boost::filesystem
        ZeroMQ::ZeroMQ
        cppzmq
        Threads::Threads
        gRPC::grpc++
)

target_include_directories(execution
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Install headers
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/execution/
    DESTINATION include/execution
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install the library
install(TARGETS execution
    EXPORT ExecutionTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)