cmake_minimum_required(VERSION 3.0)
project(SecureEpiLinker)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build libraries as SHARED" OFF)

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
add_executable (sel millionaire_prob_test.cpp
                            include/millionaire_prob.h
                            include/millionaire_prob.cpp)
add_dependencies(sel ABY)
add_dependencies(sel OTExtension)
add_dependencies(sel MIRACL)
add_dependencies(sel ENCRYPTO_utils)
add_dependencies(sel valijson_patch)


find_package(OpenSSL)
find_package(Threads)

set(EXTERNALLIBS ${SecureEpiLinker_INSTALL_PREFIX}/lib/libaby.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libotextension.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libencrypto_utils.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libmiracl_cxx.a
  ${SecureEpiLinker_INSTALL_PREFIX}/lib/libmiracl.a
  )
include_directories(SYSTEM ${SecureEpiLinker_INSTALL_PREFIX}/include)
target_include_directories(sel PRIVATE extern/valijson/include)
target_link_libraries(sel PRIVATE  ${EXTERNALLIBS} gmp gmpxx rt OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
target_link_libraries(sel PRIVATE fmt::fmt-header-only nlohmann_json)
set_property(TARGET sel PROPERTY CXX_STANDARD 17)
target_compile_options(sel PRIVATE "-Wall" "-Wpedantic" "-Wextra" "-fexceptions")
target_compile_options(sel PRIVATE "-O0" "-ggdb")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
