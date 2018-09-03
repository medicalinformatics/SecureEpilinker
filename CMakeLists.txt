cmake_minimum_required(VERSION 3.0)
project(SecureEpiLinker)
set(P ${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build libraries as SHARED" OFF)
option(${P}_MATCHING_MODE "Build matching mode capable Secure Epilinker" ON)
# inspired by https://kristerw.blogspot.com/2017/09/useful-gcc-warning-options-not-enabled.html
set(${P}_EXTRA_WARNING_FLAGS
  "-Wall" "-Wpedantic" "-Wextra"
  "-Wduplicated-cond" "-Wduplicated-branches"
  "-Wlogical-op" "-Wrestrict"
  "-Wnull-dereference"
  #"-Wold-style-cast"
  "-Wuseless-cast"
  "-Wdouble-promotion"
  #"-Wshadow"
)

# Custom compiler and linker flags
add_compile_options("-fexceptions"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
  ${CMAKE_FLAGS_EXTRA})
list(APPEND CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS_EXTRA})

set(${P}_INSTALL_PREFIX "${${P}_BINARY_DIR}/prefix")
set(${P}_DEFAULT_ARGS
  "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
  "-DCMAKE_PREFIX_PATH:PATH=${${P}_INSTALL_PREFIX};${CMAKE_PREFIX_PATH}"
  "-DCMAKE_INSTALL_PREFIX:PATH=${${P}_INSTALL_PREFIX}"
)
file(MAKE_DIRECTORY ${${P}_INSTALL_PREFIX})

find_package(OpenSSL)
find_package(Threads)

# add and configure external dependencies
add_subdirectory(extern)

include(target_link_libraries_system)

set(${P}_ABY_SOURCES
  "include/math.cpp"
  "include/util.cpp"
  "include/aby/Share.cpp"
  "include/aby/gadgets.cpp"
  "include/aby/util.cpp"
)

set(${P}_CIRCUIT_SOURCES
  ${${P}_ABY_SOURCES}
  "include/epilink_input.cpp"
  "include/circuit_config.cpp"
  "include/secure_epilinker.cpp"
  "include/clear_epilinker.cpp"
  "include/seltypes.cpp"
  "include/logger.cpp"
  "include/jsonutils.cpp"
  "include/base64.cpp"
)

set(${P}_MAIN_SOURCES
  ${${P}_CIRCUIT_SOURCES}
  "include/resourcehandler.cpp"
  "include/validator.cpp"
  "include/jsonmethodhandler.cpp"
  "include/remoteconfiguration.cpp"
  "include/localconfiguration.cpp"
  "include/connectionhandler.cpp"
  "include/configurationhandler.cpp"
  "include/databasefetcher.cpp"
  "include/datahandler.cpp"
  "include/headermethodhandler.cpp"
  "include/headerhandlerfunctions.cpp"
  "include/jsonhandlerfunctions.cpp"
  "include/resttypes.cpp"
  "include/restutils.cpp"
  "include/jsonutils.cpp"
  "include/seltypes.cpp"
  "include/serverhandler.cpp"
  "include/linkagejob.cpp"
  "include/localserver.cpp"
  "include/logger.cpp"
  "include/base64.cpp"
  "include/monitormethodhandler.cpp"
 )

# include externals as system libs to suppress warnings
#include_directories(SYSTEM "${${P}_INSTALL_PREFIX}/include")

# main target
add_executable(sel sepilinker.cpp ${${P}_MAIN_SOURCES})
target_link_libraries(sel Threads::Threads stdc++fs restbed-static
  OpenSSL::SSL OpenSSL::Crypto) # OpenSSL to fix broken restbed
target_link_libraries_system(sel
  ABY::aby curlpp_static spdlog::spdlog
  fmt::fmt-header-only nlohmann_json cxxopts)
target_include_directories(sel SYSTEM PRIVATE
  "extern/valijson/include"
  "extern/restbed/source")
target_compile_features(sel PUBLIC cxx_std_17)
target_compile_options(sel PRIVATE
  ${${P}_EXTRA_WARNING_FLAGS}
  "-fno-tree-pre"
)
target_compile_definitions(sel PRIVATE
  "$<$<BOOL:${P}_MATCHING_MODE>:SEL_MATCHING_MODE>"
  "$<$<CONFIG:Debug>:DEBUG_SEL_RESULT>"
  "$<$<CONFIG:Debug>:DEBUG_SEL_REST>"
)

# Test-Targets
# Test Secure Epilinker
add_executable(test_sel
  test/test_sel.cpp
  test/random_input_generator.cpp
  ${${P}_CIRCUIT_SOURCES})
target_link_libraries(test_sel stdc++fs)
target_link_libraries_system(test_sel ABY::aby
  fmt::fmt-header-only cxxopts nlohmann_json spdlog::spdlog)
target_compile_features(test_sel PUBLIC cxx_std_17)
target_compile_options(test_sel PRIVATE ${${P}_EXTRA_WARNING_FLAGS})
target_compile_definitions(test_sel PRIVATE
  "$<$<BOOL:${P}_MATCHING_MODE>:SEL_MATCHING_MODE>"
  "$<$<CONFIG:Debug>:DEBUG_SEL_INPUT>"
  #"DEBUG_SEL_CIRCUIT"
  "$<$<CONFIG:Debug>:DEBUG_SEL_RESULT>"
  #"DEBUG_SEL_CLEAR" # For intermediary value output in clear epilinker
  #"DEBUG_SEL_GADGETS"
)

# Test ABY Stuff
add_executable(test_aby test/test_aby.cpp ${${P}_ABY_SOURCES})
target_link_libraries_system(test_aby ABY::aby fmt::fmt-header-only cxxopts)
target_compile_features(test_aby PUBLIC cxx_std_17)
target_compile_options(test_aby PRIVATE ${${P}_EXTRA_WARNING_FLAGS})
target_compile_definitions(test_aby PRIVATE
  "$<$<CONFIG:Debug>:DEBUG_SEL_GADGETS>"
)

# Test utils
add_executable(test_util test/test_util.cpp include/util.cpp include/util.h)
target_link_libraries_system(test_util fmt::fmt-header-only)
target_compile_features(test_util PUBLIC cxx_std_17)
target_compile_options(test_util PRIVATE ${${P}_EXTRA_WARNING_FLAGS})

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
