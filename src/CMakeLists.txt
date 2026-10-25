# Core library (error, rate limiting, retry, units)
#
# Glaze is header-only and pulled in via include-path only (matching how
# nlohmann was integrated pre-migration). This sidesteps CMake's "INTERFACE
# target not in export set" error that fires if we link glaze::glaze
# directly onto these installable static libs.
add_library(nws_core STATIC
    core/error.cpp
    core/rate_limit.cpp
    core/retry.cpp
    core/units.cpp
)
target_compile_features(nws_core PUBLIC cxx_std_23)
target_include_directories(nws_core PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# HTTP client library
add_library(nws_http STATIC
    http/client.cpp
)
target_link_libraries(nws_http PUBLIC nws_core CURL::libcurl)
target_include_directories(nws_http PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Models library
add_library(nws_models STATIC
    models/common.cpp
    models/glaze_detail.cpp
    models/point.cpp
    models/forecast.cpp
    models/gridpoint.cpp
    models/observation.cpp
    models/station.cpp
    models/alert.cpp
    models/zone.cpp
    models/office.cpp
    models/product.cpp
    models/cli_report.cpp
    models/nbp_bulletin.cpp
    models/aviation.cpp
    models/radar.cpp
    models/glossary.cpp
)
target_link_libraries(nws_models PUBLIC nws_core)
target_include_directories(nws_models PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Typed API client
add_library(nws_api STATIC
    api/client.cpp
)
target_link_libraries(nws_api PUBLIC nws_core nws_http nws_models)
target_include_directories(nws_api PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Main SDK library (combines all)
add_library(nws INTERFACE)
add_library(nws::nws ALIAS nws)
target_link_libraries(nws INTERFACE nws_core nws_http nws_models nws_api)

# Glaze never appears in public headers or the installed package.
foreach(target nws_core nws_models nws_api)
    target_include_directories(${target} SYSTEM PRIVATE ${glaze_SOURCE_DIR}/include)
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/src)
endforeach()

if(WIN32)
    target_compile_definitions(nws_http PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
