add_executable(bench bench.cpp)

target_link_libraries(bench
    PRIVATE
    itch_lib
    benchmark::benchmark_main
)