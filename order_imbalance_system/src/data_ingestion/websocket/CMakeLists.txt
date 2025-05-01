# WebSocket components for data ingestion layer
set(WEBSOCKET_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/client_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/connection_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/exchanges/binance_websocket_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/exchanges/binance_client_factory.cpp
)

set(WEBSOCKET_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/data_ingestion/websocket/client_base.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/data_ingestion/websocket/connection_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/data_ingestion/websocket/exchanges/binance_websocket_client.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/data_ingestion/websocket/exchanges/binance_client_factory.hpp
)

# Add the WebSocket sources to the data ingestion library
target_sources(data_ingestion_lib
    PRIVATE
        ${WEBSOCKET_SOURCES}
        ${WEBSOCKET_HEADERS}
)

# Add example executables
add_executable(websocket_example
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/websocket_example.cpp
)
target_link_libraries(websocket_example
    PRIVATE
        data_ingestion_lib
        Boost::system
        Boost::thread
        OpenSSL::SSL
)

add_executable(exchange_manager_example
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/exchange_manager_example.cpp
)
target_link_libraries(exchange_manager_example
    PRIVATE
        data_ingestion_lib
        Boost::system
        Boost::thread
        OpenSSL::SSL
)

# Install targets
install(TARGETS
    websocket_example
    exchange_manager_example
    RUNTIME DESTINATION bin
)