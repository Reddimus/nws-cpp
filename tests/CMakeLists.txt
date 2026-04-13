add_executable(nws_tests
    test_error.cpp
    test_units.cpp
    test_rate_limit.cpp
    test_retry.cpp
    test_models.cpp
)
target_link_libraries(nws_tests PRIVATE
    nws_core nws_http nws_models nws_api
    GTest::gtest_main GTest::gmock
)
target_include_directories(nws_tests PRIVATE ${json_SOURCE_DIR}/include)
include(GoogleTest)
gtest_discover_tests(nws_tests)
