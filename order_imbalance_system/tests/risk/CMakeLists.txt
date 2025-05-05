# CMakeLists.txt for Risk tests

# Find required packages
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Include directories for tests
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Risk unit test sources
set(RISK_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_risk_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/test_limit_manager.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/test_risk_calculations.cpp
)

# Risk integration test sources
set(RISK_INTEGRATION_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_risk_system.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_risk_service.cpp
)

# Define risk test executables
add_executable(risk_unit_tests ${RISK_TEST_SOURCES})
add_executable(risk_integration_tests ${RISK_INTEGRATION_TEST_SOURCES})
add_executable(test_risk_manager ${CMAKE_CURRENT_SOURCE_DIR}/test_risk_manager.cpp)
add_executable(test_risk_limits ${CMAKE_CURRENT_SOURCE_DIR}/test_risk_limits.cpp)
# add_executable(test_risk_calculations ${CMAKE_CURRENT_SOURCE_DIR}/test_risk_calculations.cpp)

# Common libraries for risk tests
set(COMMON_LIBRARIES
    risk_management
    data_ingestion
    GTest::GTest
    GTest::Main
    GMock::GMock
    Boost::system
    Boost::thread
    OpenSSL::SSL
)

# Link libraries for risk test executables
target_link_libraries(risk_unit_tests PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(risk_integration_tests PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(test_risk_manager PRIVATE ${COMMON_LIBRARIES})
target_link_libraries(test_risk_limits PRIVATE ${COMMON_LIBRARIES})
# target_link_libraries(test_risk_calculations PRIVATE ${COMMON_LIBRARIES})

# Add include directories for all risk targets
foreach(target 
    risk_unit_tests
    risk_integration_tests
    test_risk_manager
    test_risk_limits
    # test_risk_calculations
)
    target_include_directories(${target}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endforeach()

# Register all risk tests with CTest
include(GoogleTest)
gtest_discover_tests(risk_unit_tests)
gtest_discover_tests(risk_integration_tests)
gtest_discover_tests(test_risk_manager)
gtest_discover_tests(test_risk_limits)
# gtest_discover_tests(test_risk_calculations)

# Install risk test targets
install(TARGETS
    risk_unit_tests
    risk_integration_tests
    test_risk_manager
    test_risk_limits
    # test_risk_calculations
    RUNTIME DESTINATION bin/tests/risk
)
