file(GLOB_RECURSE LIB_SOURCES "*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(itch_lib STATIC ${LIB_SOURCES})
target_include_directories(itch_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_options(itch_lib PRIVATE -O3)
target_link_libraries(itch_lib PUBLIC project_options)
target_link_libraries(itch_lib PUBLIC unordered_dense::unordered_dense)
set_property(TARGET itch_lib PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
