cmake_minimum_required(VERSION 3.20)
project(nws-cpp VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Options
option(NWS_BUILD_TESTS "Build tests" ON)
option(NWS_BUILD_EXAMPLES "Build examples" ON)
option(NWS_ENABLE_LTO "Enable Link Time Optimization" ON)
option(NWS_NATIVE_ARCH "Use -march=native for CPU-specific tuning" OFF)

# Aggressive optimization flags for Release builds. MSVC has its own
# Release defaults; only override for GCC/Clang.
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Enable LTO if requested and supported
if(NWS_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
    if(LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        message(STATUS "LTO enabled for Release builds")
    else()
        message(WARNING "LTO not supported: ${LTO_ERROR}")
    endif()
endif()

# CPU-specific tuning. GCC/Clang only.
if(NOT MSVC)
    if(NWS_NATIVE_ARCH)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
        message(STATUS "Using -march=native for CPU-specific optimizations")
    else()
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-march=x86-64-v3" COMPILER_SUPPORTS_X86_64_V3)
        if(COMPILER_SUPPORTS_X86_64_V3)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=x86-64-v3")
        else()
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mtune=generic")
        endif()
    endif()
endif()

# Sanitizer support
option(NWS_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSanitizer" OFF)
if(NWS_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "Sanitizers enabled (ASan + UBSan)")
endif()

# Code coverage support
option(NWS_ENABLE_COVERAGE "Enable code coverage instrumentation (gcov)" OFF)
if(NWS_ENABLE_COVERAGE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
    message(STATUS "Code coverage enabled")
endif()

# Find dependencies
find_package(CURL REQUIRED)

include(FetchContent)

# Glaze - JSON library (compile-time reflection, ~4x parse speedup over
# nlohmann on the NWS gridpoint-forecast hot path, which is what consumers
# like kalshi-trader and polymarket-trader hit on the 60+-layer payload).
# License: MIT.
FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v7.6.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)

if(NWS_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NWS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install targets
set(NWS_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/nws")

install(TARGETS nws_core nws_http nws_models nws_api nws
    EXPORT nwsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/nws DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/nwsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/nwsConfig.cmake
    INSTALL_DESTINATION ${NWS_INSTALL_CMAKEDIR}
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/nwsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
export(EXPORT nwsTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/nwsTargets.cmake
    NAMESPACE nws::
)
install(EXPORT nwsTargets
    FILE nwsTargets.cmake
    NAMESPACE nws::
    DESTINATION ${NWS_INSTALL_CMAKEDIR}
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/nwsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/nwsConfigVersion.cmake
    DESTINATION ${NWS_INSTALL_CMAKEDIR}
)
