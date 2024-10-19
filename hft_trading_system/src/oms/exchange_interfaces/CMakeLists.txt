# Create a library for the exchange interfaces module
add_library(exchange_interfaces_module binance_interface.cpp
                                        coinbase_interface.cpp)

# Include directories for this module
target_include_directories(exchange_interfaces_module PUBLIC ${CMAKE_SOURCE_DIR}/include)
