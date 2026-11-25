# Latency/throughput harnesses (see docs/perf/PLAN.md)
add_executable(replay_bench replay_bench.cpp)
target_link_libraries(replay_bench
    PRIVATE
        data_ingestion
        strategy
        common
        nlohmann_json::nlohmann_json
        Threads::Threads
)
# strategy headers include siblings relative to include/strategy
target_include_directories(replay_bench PRIVATE ${CMAKE_SOURCE_DIR}/include/strategy)
