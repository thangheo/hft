# CMakeLists.txt for Risk Interface Tests

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add test executables
add_executable(strategy_interface_test 
    strategy_interface_test.cpp
    ${PROJECT_SOURCE_DIR}/src/risk/interfaces/strategy_interface.cpp
)

add_executable(execution_interface_test 
    execution_interface_test.cpp
    ${PROJECT_SOURCE_DIR}/src/risk/interfaces/execution_interface.cpp
)

add_executable(data_interface_test 
    data_interface_test.cpp
    ${PROJECT_SOURCE_DIR}/src/risk/interfaces/data_interface.cpp
)

# Link libraries
target_link_libraries(strategy_interface_test 
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES}
    pthread
    risk_common
    zmq
)
target_link_libraries(execution_interface_test 
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES}
    pthread
    risk_common
    zmq
)

target_link_libraries(data_interface_test 
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES}
    pthread
    risk_common
    zmq
)

# Add tests to CTest
add_test(NAME StrategyInterfaceTest COMMAND strategy_interface_test)
add_test(NAME ExecutionInterfaceTest COMMAND execution_interface_test)
add_test(NAME DataInterfaceTest COMMAND data_interface_test)

# Set test properties
set_tests_properties(StrategyInterfaceTest PROPERTIES TIMEOUT 10)
set_tests_properties(ExecutionInterfaceTest PROPERTIES TIMEOUT 10)
set_tests_properties(DataInterfaceTest PROPERTIES TIMEOUT 10)