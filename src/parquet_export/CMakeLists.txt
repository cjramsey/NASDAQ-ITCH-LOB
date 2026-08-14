find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

add_library(parquet_export_lib
    ${CMAKE_SOURCE_DIR}/src/types.cpp
    ${CMAKE_SOURCE_DIR}/src/parser.cpp
    parquet_writer.cpp
)
target_include_directories(parquet_export_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(parquet_export_lib PUBLIC BUILD_PARQUET_EXPORT)
target_link_libraries(parquet_export_lib PUBLIC Parquet::parquet_shared Arrow::arrow_shared)
target_compile_options(parquet_export_lib PRIVATE -O3 -Wno-unused-result)

add_executable(itch_to_parquet parquet_export_main.cpp)
target_link_libraries(itch_to_parquet PRIVATE parquet_export_lib)
target_compile_options(itch_to_parquet PRIVATE -O3 -march=native)

set_target_properties(itch_to_parquet PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
