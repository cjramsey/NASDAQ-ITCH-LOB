cmake_minimum_required(VERSION 3.20)

project(ITCH_Parser VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(project_options INTERFACE)
target_compile_options(project_options INTERFACE -march=native -Wno-interference-size)

add_subdirectory(src)

add_executable(direct src/main.cpp)
add_executable(ring_buffer src/main.cpp)
add_executable(direct_perf src/main.cpp)
add_executable(ring_buffer_perf src/main.cpp)

foreach(TARGET direct ring_buffer direct_perf ring_buffer_perf)
    target_link_libraries(${TARGET} PRIVATE itch_lib)
endforeach()

target_compile_options(direct PRIVATE -O3)
target_compile_options(ring_buffer PRIVATE -O3)
target_compile_definitions(ring_buffer PRIVATE USE_RING_BUFFER)

target_compile_options(direct_perf PRIVATE -O2 -g -fno-omit-frame-pointer)
target_compile_options(ring_buffer_perf PRIVATE -O2 -g -fno-omit-frame-pointer)
target_compile_definitions(ring_buffer_perf PRIVATE USE_RING_BUFFER)

foreach(TARGET direct ring_buffer)
    set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endforeach()

if(PROJECT_IS_TOP_LEVEL)
    option(BUILD_TESTS      "Build tests"       ON)
    option(BUILD_BENCHMARKS "Build benchmarks"  ON)
endif()

include(cmake/FetchDependencies.cmake)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()