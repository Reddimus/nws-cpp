add_executable(nws_tests
    test_error.cpp
    test_units.cpp
    test_rate_limit.cpp
    test_retry.cpp
    test_models.cpp
    test_pagination.cpp
    test_cache.cpp
    test_api.cpp
)
target_link_libraries(nws_tests PRIVATE
    nws_core nws_http nws_models nws_api
    GTest::gtest_main GTest::gmock
)
include(GoogleTest)
gtest_discover_tests(nws_tests)

# Glaze deserializer tests — verifies the migration's parse output matches
# the documented NWS response shape and the pre-migration null-safety
# behaviour on the GeoJSON variant geometry, QuantitativeValue null-value,
# and dynamic alert-count maps.
add_executable(nws_glaze_test glaze_test.cpp)
target_link_libraries(nws_glaze_test PRIVATE
    GTest::gtest_main nws_models
)
add_test(NAME nws_glaze_test COMMAND nws_glaze_test)

# Glaze parse-throughput regression guard. The bench parses a representative
# NWS gridpoint-forecast response 1k times and asserts us/op stays under a
# cap. ctest --timeout = 30s; actual wall-clock is ~50-150 ms on x86_64-v3,
# so the timeout fires only on catastrophic regression.
add_executable(nws_parse_benchmark parse_benchmark.cpp)
target_link_libraries(nws_parse_benchmark PRIVATE nws_models)
add_test(NAME nws_parse_benchmark COMMAND nws_parse_benchmark)
set_tests_properties(nws_parse_benchmark PROPERTIES TIMEOUT 30)
