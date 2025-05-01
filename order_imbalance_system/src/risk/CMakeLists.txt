set(RISK_SOURCES
    # Pre-trade risk controls
    pretrade/exposure_calculator.cpp
    pretrade/fat_finger_protection.cpp
    pretrade/order_validator.cpp
    pretrade/position_limit_checker.cpp
    pretrade/pre_trade_risk_filter.cpp
    
    # Post-trade risk controls
    posttrade/balance_reconciliation.cpp
    posttrade/circuit_breaker.cpp
    posttrade/drawdown_monitor.cpp
    posttrade/post_trade_monitor.cpp
    posttrade/stop_loss_manager.cpp
    
    # Portfolio management
    portfolio/collateral_manager.cpp
    portfolio/portfolio_manager.cpp
    portfolio/position_tracker.cpp
    portfolio/rebalancing_engine.cpp
    
    # Interfaces
    interfaces/data_interface.cpp
    interfaces/execution_interface.cpp
    interfaces/strategy_interface.cpp
    
    # Services
    service/risk_service.cpp
    service/risk_validation_service.cpp
    
    # Utilities
    utils/limit_manager.cpp
    utils/risk_logger.cpp
    utils/risk_metrics.cpp
)

add_library(risk ${RISK_SOURCES})

target_link_libraries(risk
    PUBLIC
        common
        execution
        proto_lib
        Boost::system
        Boost::filesystem
        ZeroMQ::ZeroMQ
        cppzmq
        Threads::Threads
)

target_include_directories(risk
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Install headers
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/risk/
    DESTINATION include/risk
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install the library
install(TARGETS risk
    EXPORT RiskTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)