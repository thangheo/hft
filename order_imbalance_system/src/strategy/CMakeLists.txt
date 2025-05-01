set(STRATEGY_SOURCES
    adaptive_mechanism.cpp
    order_book_analyzer.cpp
    imbalance_signal_generator.cpp
    liquidity_flow_monitor.cpp
    ab_testing_framework.cpp
    volume_profile_analyzer.cpp
    imbalance_parameter_tuner.cpp
    strategy_metrics.cpp
    exchange_calibration.cpp
    momentum_correlation.cpp
    strategy_config_loader.cpp
    signal_decay_model.cpp
    strategy_engine.cpp
    threshold_manager.cpp
    imbalance_detection_engine.cpp
)

add_library(strategy ${STRATEGY_SOURCES})

target_link_libraries(strategy
    PUBLIC
        common
        data_ingestion
        proto_lib
        Boost::system
        Boost::filesystem
        ZeroMQ::ZeroMQ
        cppzmq
        Threads::Threads
)

target_include_directories(strategy
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Install headers
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/strategy/
    DESTINATION include/strategy
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install the library
install(TARGETS strategy
    EXPORT StrategyTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)