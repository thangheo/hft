# Find Google Test
find_package(GTest REQUIRED)

# Include directories for tests
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Data Ingestion Tests
set(DATA_INGESTION_TEST_SOURCES
    data_ingestion/test_order_book.cpp
    data_ingestion/order_book_reconstructor_tests.cpp
    data_ingestion/websocket_client_tests.cpp
    data_ingestion/mock_exchange_server.cpp
)

add_executable(data_ingestion_tests ${DATA_INGESTION_TEST_SOURCES})
target_link_libraries(data_ingestion_tests
    PRIVATE
        data_ingestion
        common
        ${GTEST_BOTH_LIBRARIES}
        Threads::Threads
)
add_test(NAME data_ingestion_tests COMMAND data_ingestion_tests)

# Strategy Tests
set(STRATEGY_TEST_SOURCES
    strategy/test_order_book_analyzer.cpp
    strategy/test_imbalance_detection.cpp
    strategy/test_signal_generation.cpp
    strategy/test_integration_tests.cpp
)

add_executable(strategy_tests ${STRATEGY_TEST_SOURCES})
target_link_libraries(strategy_tests
    PRIVATE
        strategy
        data_ingestion
        common
        ${GTEST_BOTH_LIBRARIES}
        Threads::Threads
)
add_test(NAME strategy_tests COMMAND strategy_tests)

# Execution Tests
set(EXECUTION_TEST_SOURCES
    execution/test_api_client_manager.cpp
    execution/test_execution_algorithms.cpp
    execution/test_order_generator.cpp
    execution/test_order_lifecycle.cpp
    execution/test_order_type_selector.cpp
    execution/test_rate_limit_controller.cpp
    execution/test_smart_order_router.cpp
    execution/integration_test_execution_engine.cpp
)

add_executable(execution_tests ${EXECUTION_TEST_SOURCES})
target_link_libraries(execution_tests
    PRIVATE
        execution
        common
        ${GTEST_BOTH_LIBRARIES}
        Threads::Threads
)
add_test(NAME execution_tests COMMAND execution_tests)

# Risk Tests - Organized by subdirectories
set(RISK_PRETRADE_TEST_SOURCES
    risk/pretrade/test_exposure_calculator.cpp
    risk/pretrade/test_fat_finger_protection.cpp
    risk/pretrade/test_order_validator.cpp
    risk/pretrade/test_position_limit_checker.cpp
)

add_executable(risk_pretrade_tests ${RISK_PRETRADE_TEST_SOURCES})
target_link_libraries(risk_pretrade_tests
    PRIVATE
        risk
        common
        ${GTEST_BOTH_LIBRARIES}
        Threads::Threads
)
add_test(NAME risk_pretrade_tests COMMAND risk_pretrade_tests)

set(RISK_POSTTRADE_TEST_SOURCES
    risk/posttrade/test_balance_reconciliation.cpp
    risk/posttrade/test_circuit_breaker.cpp
    risk/posttrade/test_drawdown_monitor.cpp
    risk/posttrade/test_stop_loss_manager.cpp
)

add_executable(risk_posttrade_tests ${RISK_POSTTRADE_TEST_SOURCES})
target_link_libraries(risk_posttrade_tests
    PRIVATE
        risk
        common
        ${GTEST_BOTH_LIBRARIES}
        Threads::Threads
)
add_test(NAME risk_posttrade_tests COMMAND risk_posttrade_tests)

set(RISK_PORTFOLIO_TEST_SOURCES
    risk/portfolio/test_collateral_manager.cpp
    risk/portfolio/test_position_tracker.cpp
    risk/portfolio/test_rebalancing_engine.cpp
)

add_executable(risk_portfolio_tests ${RISK_PORTFOLIO_TEST_SOURCES})
target_link_libraries(risk_portfolio_tests
    PRIVATE
        risk
        common
        ${GTEST_BOTH_LIBRARIES}
        Threads::Threads
)
add_test(NAME risk_portfolio_tests COMMAND risk_portfolio_tests)

set(RISK_INTEGRATION_TEST_SOURCES
    risk/integration/test_portfolio_manager.cpp
    risk/integration/test_post_trade_monitor.cpp
    risk/integration/test_pretrade_risk_filter.cpp
    risk/integration/test_risk_service.cpp
    risk/integration/test_risk_system.cpp
)

add_executable(risk_integration_tests ${RISK_INTEGRATION_TEST_SOURCES})
target_link_libraries(risk_integration_tests
    PRIVATE
        risk
        common
        ${GTEST_BOTH_LIBRARIES}
        Threads::Threads
)
add_test(NAME risk_integration_tests COMMAND risk_integration_tests)

# Python tests - We'll use CMake to run Python unit tests
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
    NAME python_monitoring_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tests/monitoring
)

add_test(
    NAME python_alerts_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tests/alerts
)

add_test(
    NAME python_api_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tests/api
)

add_test(
    NAME python_system_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tests/system
)

# Code coverage target
if(ENABLE_COVERAGE AND CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    append_coverage_compiler_flags()
    
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE ctest
        EXCLUDE "/usr/*" "${CMAKE_SOURCE_DIR}/tests/*" "${CMAKE_BINARY_DIR}/*"
        DEPENDENCIES data_ingestion_tests strategy_tests execution_tests 
                     risk_pretrade_tests risk_posttrade_tests 
                     risk_portfolio_tests risk_integration_tests
    )
endif()