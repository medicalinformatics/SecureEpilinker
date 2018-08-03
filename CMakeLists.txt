cmake_minimum_required(VERSION 3.0)
project(SecureEpiLinker)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build libraries as SHARED" OFF)
option(BUILD_ABY_EXE "Build executable tests for ABY" OFF)

set(SecureEpiLinker_INSTALL_PREFIX "${SecureEpiLinker_BINARY_DIR}/prefix")
set(SecureEpiLinker_DEFAULT_ARGS
    "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
    "-DCMAKE_PREFIX_PATH:PATH=${SecureEpiLinker_INSTALL_PREFIX};${CMAKE_PREFIX_PATH}"
    "-DCMAKE_INSTALL_PREFIX:PATH=${SecureEpiLinker_INSTALL_PREFIX}"
)
file(MAKE_DIRECTORY ${SecureEpiLinker_INSTALL_PREFIX})

# build external dependencies

add_subdirectory(extern)

# build main application
set(EPIRESTSRC
      include/resourcehandler.cpp
      include/validator.cpp
      include/jsonmethodhandler.cpp
      include/remoteconfiguration.cpp
      include/localconfiguration.cpp
      include/connectionhandler.cpp
      include/configurationhandler.cpp
      include/databasefetcher.cpp
      include/datahandler.cpp
      include/jsonhandlerfunctions.cpp
      include/resttypes.cpp
      include/seltypes.cpp
      include/serverhandler.cpp
      include/linkagejob.cpp
      include/localserver.cpp
      include/logger.cpp
      include/base64.cpp
      include/monitormethodhandler.cpp
      include/tempselmethodhandler.cpp)

    set(EPIRESTHEAD
      include/resourcehandler.h
      include/validator.h
      include/methodhandler.hpp
      include/jsonmethodhandler.h
      include/remoteconfiguration.h
      include/localconfiguration.h
      include/authenticationconfig.hpp
      include/apikeyconfig.hpp
      include/connectionhandler.h
      include/configurationhandler.h
      include/databasefetcher.h
      include/datahandler.h
      include/jsonhandlerfunctions.h
      include/resttypes.h
      include/seltypes.h
      include/serverhandler.h
      include/linkagejob.h
      include/localserver.h
      include/logger.h
      include/base64.h
      include/monitormethodhandler.h
      include/tempselmethodhandler.h)

set(SELABYSOURCES "include/math.cpp" "include/util.cpp"
  "include/aby/Share.cpp" "include/aby/gadgets.cpp" "include/aby/util.cpp")

set(SELSOURCES ${SELABYSOURCES}
  "include/epilink_input.cpp" "include/secure_epilinker.cpp"
  "include/clear_epilinker.cpp"
  "include/seltypes.cpp")

set(SELABYHEADER "include/math.h" "include/util.h"
  "include/aby/Share.h" "include/aby/gadgets.h"
  "include/aby/util.h")

set(SELHEADER ${SELABYHEADER}
  "include/epilink_input.h" "include/secure_epilinker.h"
  "include/clear_epilinker.h"
  "include/seltypes.h")

add_executable (sel sepilinker.cpp ${EPIRESTSRC} ${EPIRESTHEAD} ${SELSOURCES} ${SELHEADER})
add_dependencies(sel ABY)
add_dependencies(sel OTExtension)
add_dependencies(sel MIRACL)
add_dependencies(sel ENCRYPTO_utils)
add_dependencies(sel valijson_patch)
# FIXME(TK): restbed patch not working. Needs to be applied manually
message(WARNING "Restbed Patch needs to be applied manually at the moment")
add_dependencies(sel restbed_patch)

set(EXTERNALLIBS ${SecureEpiLinker_INSTALL_PREFIX}/lib/libaby.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libotextension.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libencrypto_utils.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libmiracl_cxx.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libmiracl.a
  )

find_package(OpenSSL)
find_package(Threads)

#include externals as system libs to suppress warnings
include_directories(SYSTEM ${SecureEpiLinker_INSTALL_PREFIX}/include
  "extern/restbed/source")
target_include_directories(sel PRIVATE extern/valijson/include)
target_include_directories(sel PRIVATE extern/curlpp/include)
target_include_directories(sel PRIVATE extern/spdlog/include)
# Debug Version (sanitizers linked)
target_link_libraries(sel PRIVATE ${EXTERNALLIBS} gmp gmpxx rt
  OpenSSL::SSL OpenSSL::Crypto Threads::Threads stdc++fs curl curlpp
  ${PROJECT_BINARY_DIR}/extern/restbed/librestbed.a)
# Release Version
#target_link_libraries(sel PRIVATE ${EXTERNALLIBS} gmp gmpxx rt
  #OpenSSL::SSL OpenSSL::Crypto Threads::Threads stdc++fs
  #${PROJECT_BINARY_DIR}/extern/restbed/librestbed.a)
target_link_libraries(sel PRIVATE fmt::fmt-header-only nlohmann_json
  cxxopts)
set_property(TARGET sel PROPERTY CXX_STANDARD 17)
target_compile_options(sel PRIVATE "-Wall" "-Wpedantic" "-Wextra" "-fexceptions")
#Debug Version
target_compile_options(sel PRIVATE "-O0" "-ggdb" "-g3"
  "-DDEBUG_SEL_CLEAR"
  "-DDEBUG_SEL_RESULT"
  "-DDEBUG_SEL_REST")
#target_compile_options(sel PRIVATE "-O2" "-s" "-DNDEBUG"
  #"-fsanitize-address-use-after-scope" "-fsanitize=undefined")

# Test-Targets
# Test ABY Stuff
add_executable(test_aby test/test_aby.cpp ${SELABYSOURCES} ${SELABYHEADER})
add_dependencies(test_aby ABY)
add_dependencies(test_aby OTExtension)
add_dependencies(test_aby MIRACL)
add_dependencies(test_aby ENCRYPTO_utils)
target_link_libraries(test_aby PRIVATE ${EXTERNALLIBS} gmp gmpxx rt
  OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
target_link_libraries(test_aby PRIVATE fmt::fmt-header-only cxxopts)
set_property(TARGET test_aby PROPERTY CXX_STANDARD 17)
target_compile_options(test_aby PRIVATE "-O0" "-ggdb" "-g3"
  "-DDEBUG_SEL_GADGETS")

# Test Secure Epilinker
add_executable(test_sel test/test_sel.cpp ${SELSOURCES} ${SELHEADER})
add_dependencies(test_sel ABY)
add_dependencies(test_sel OTExtension)
add_dependencies(test_sel MIRACL)
add_dependencies(test_sel ENCRYPTO_utils)
target_link_libraries(test_sel PRIVATE ${EXTERNALLIBS} gmp gmpxx rt
  OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
target_link_libraries(test_sel PRIVATE fmt::fmt-header-only cxxopts)
set_property(TARGET test_sel PROPERTY CXX_STANDARD 17)
target_compile_options(test_sel PRIVATE "-O0" "-ggdb" "-g3"
  "-DDEBUG_SEL_INPUT"
  #"-DDEBUG_SEL_CIRCUIT"
  "-DDEBUG_SEL_RESULT"
  #"-DDEBUG_SEL_CLEAR" # For intermediary value output in clear epilinker
  #"-DDEBUG_SEL_GADGETS"
  #"-DSEL_MATCHING_MODE" # Compile matching mode logic
  )

# Test utils
add_executable(test_util test/test_util.cpp include/util.cpp include/util.h)
target_link_libraries(test_util PRIVATE fmt::fmt-header-only)
set_property(TARGET test_util PROPERTY CXX_STANDARD 17)
target_compile_options(test_aby PRIVATE "-O0" "-ggdb" "-g3")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
