message(STATUS "Adding external projects")

option(CXXOPTS_BUILD_EXAMPLES "cxxopts Examples (Overwrite)" OFF)
option(JSON_BuildTests "nlohmann/json Tests (Overwrite)" OFF)
option(valijson_BUILD_TESTS "valijson Tests (Overwrite)" OFF)
option(valijson_BUILD_EXAMPLES "valijson Examples (Overwrite)" OFF)
option(SPDLOG_BUILD_BENCH "spdlog Benchmarks (Overwrite)" OFF)
option(SPDLOG_BUILD_EXAMPLES "spdlog Examples (Overwrite)" OFF)
option(SPDLOG_BUILD_TESTING "spdlog Tests (Overwrite)" OFF)
# Make spdlog use the project's (header-only) fmt instead of its bundled copy,
# so spdlog and the rest of the codebase share exactly one fmt version.
# Relies on add_subdirectory(fmt) running before add_subdirectory(spdlog) below.
set(SPDLOG_FMT_EXTERNAL_HO ON CACHE BOOL "spdlog uses external header-only fmt (Overwrite)" FORCE)

# patch nlohmann/json include path and CMake option names
execute_process(COMMAND
  "${PROJECT_SOURCE_DIR}/cmake/patch-valijson.sh"
  "${PROJECT_SOURCE_DIR}/extern/valijson")

add_subdirectory(ABY)
add_subdirectory(fmt)
add_subdirectory(cxxopts)
add_subdirectory(json EXCLUDE_FROM_ALL)
add_subdirectory(valijson EXCLUDE_FROM_ALL)
add_subdirectory(restbed EXCLUDE_FROM_ALL)
add_subdirectory(curlpp EXCLUDE_FROM_ALL)
add_subdirectory(spdlog EXCLUDE_FROM_ALL)

target_compile_options(aby PRIVATE "-w")
target_compile_options(encrypto_utils PRIVATE "-w")
target_compile_options(otextension PRIVATE "-w")
target_compile_options(restbed-static PRIVATE "-w")
