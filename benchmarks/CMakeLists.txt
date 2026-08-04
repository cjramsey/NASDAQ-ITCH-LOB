add_executable(bench bench.cpp)

set_target_properties(bench PROPERTIES
    COMPILE_OPTIONS "-O3"
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

target_link_libraries(bench
    PRIVATE
    itch_lib
    benchmark::benchmark_main
)
