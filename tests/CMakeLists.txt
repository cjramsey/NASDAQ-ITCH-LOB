add_executable(tests lob_test.cpp)

target_link_libraries(tests
    PRIVATE
    itch_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)