# src/risk/CMakeLists.txt

# Collect all source files
file(GLOB_RECURSE RISK_SOURCES
    "*.cpp"
    "interfaces/*.cpp"
    "portfolio/*.cpp"
    "posttrade/*.cpp"
    "pretrade/*.cpp"
    "service/*.cpp"
    "utils/*.cpp"
)

# Excluded: written against an older RiskConfig/RiskEvent/TradingSignal API and
# not referenced by any other component. Re-enable once ported to the current API.
list(FILTER RISK_SOURCES EXCLUDE REGEX ".*/risk_manager\\.cpp$")
list(FILTER RISK_SOURCES EXCLUDE REGEX ".*/utils/limit_manager\\.cpp$")
list(FILTER RISK_SOURCES EXCLUDE REGEX ".*/service/risk_service\\.cpp$")
list(FILTER RISK_SOURCES EXCLUDE REGEX ".*/service/risk_validation_service\\.cpp$")
# Example program with its own main(); must not be part of the library
list(FILTER RISK_SOURCES EXCLUDE REGEX ".*/interfaces/data_interface_examples\\.cpp$")

# Define library
add_library(risk ${RISK_SOURCES})

# Include directories
target_include_directories(risk
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include/risk
)

# Link libraries
target_link_libraries(risk
    PUBLIC
        common
        execution
    PRIVATE
        strategy
        data_ingestion
        nlohmann_json::nlohmann_json
)