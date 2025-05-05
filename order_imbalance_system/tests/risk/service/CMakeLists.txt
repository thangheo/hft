# Tests for Risk Management System
# Add to CMakeLists.txt

# Google Test dependency
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Google Mock dependency
find_package(GMock REQUIRED)
include_directories(${GMOCK_INCLUDE_DIRS})

# JSON for Modern C++ (for testing JSON processing)
find_package(nlohmann_json REQUIRED)

# Test executables
add_executable(test_risk_validation_service 
    tests/risk/service/test_risk_validation_service.cpp)
target_link_libraries(test_risk_validation_service
    ${PROJECT_NAME}_risk
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    pthread)

add_executable(test_risk_service 
    tests/risk/service/test_risk_service.cpp)
target_link_libraries(test_risk_service
    ${PROJECT_NAME}_risk
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread)

add_executable(test_risk_manager 
    tests/risk/test_risk_manager.cpp)
target_link_libraries(test_risk_manager
    ${PROJECT_NAME}_risk
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    pthread)

add_executable(test_risk_system_integration 
    tests/risk/integration/test_risk_system_integration.cpp)
target_link_libraries(test_risk_system_integration
    ${PROJECT_NAME}_risk
    ${GTEST_LIBRARIES}
    pthread)

# Add tests to CTest
add_test(NAME RiskValidationServiceTest COMMAND test_risk_validation_service)
add_test(NAME RiskServiceTest COMMAND test_risk_service)
add_test(NAME RiskManagerTest COMMAND test_risk_manager)
add_test(NAME RiskSystemIntegrationTest COMMAND test_risk_system_integration)