cmake_minimum_required(VERSION 3.20)

project(ITCH_Parser VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(direct ${SOURCES})
add_executable(ring_buffer ${SOURCES})
add_executable(direct_perf ${SOURCES})
add_executable(ring_buffer_perf ${SOURCES})

foreach(TARGET direct ring_buffer direct_perf ring_buffer_perf)
    target_include_directories(${TARGET} PRIVATE include)
endforeach()

target_compile_options(direct PRIVATE -O3 -march=native -Wno-interference-size)

target_compile_options(ring_buffer PRIVATE -O3 -march=native -Wno-interference-size)
target_compile_definitions(ring_buffer PRIVATE USE_RING_BUFFER)

target_compile_options(direct_perf PRIVATE -O2 -g -march=native -Wno-interference-size -fno-omit-frame-pointer)

target_compile_options(ring_buffer_perf PRIVATE -O2 -g -march=native -Wno-interference-size -fno-omit-frame-pointer)
target_compile_definitions(ring_buffer_perf PRIVATE USE_RING_BUFFER)