add_executable(bench bench.cpp)

set_target_properties(benchmark PROPERTIES
    COMPILE_OPTIONS "-O3"
)

target_link_libraries(bench
    PRIVATE
    itch_lib
    benchmark::benchmark_main
)
