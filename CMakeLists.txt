cmake_minimum_required(VERSION 3.0)
project(Project)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build libraries as SHARED" OFF)

set(Project_INSTALL_PREFIX "${Project_BINARY_DIR}/prefix")
set(Project_DEFAULT_ARGS
    "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
    "-DCMAKE_PREFIX_PATH:PATH=${Project_INSTALL_PREFIX};${CMAKE_PREFIX_PATH}"
    "-DCMAKE_INSTALL_PREFIX:PATH=${Project_INSTALL_PREFIX}"
)
file(MAKE_DIRECTORY ${Project_INSTALL_PREFIX})

# build external dependencies
add_subdirectory(extern)

# build main application
add_executable (millionaire millionaire_prob_test.cpp
                            include/millionaire_prob.h
                            include/millionaire_prob.cpp)
add_dependencies(millionaire ABY)
add_dependencies(millionaire OTExtension)
add_dependencies(millionaire MIRACL)
add_dependencies(millionaire ENCRYPTO_utils)


find_package(OpenSSL)
find_package(Threads)

set(EXTERNALLIBS ${Project_INSTALL_PREFIX}/lib/libaby.a
  ${Project_INSTALL_PREFIX}/lib/libotextension.a
  ${Project_INSTALL_PREFIX}/lib/libencrypto_utils.a
  ${Project_INSTALL_PREFIX}/lib/libmiracl_cxx.a
  ${Project_INSTALL_PREFIX}/lib/libmiracl.a
  )

target_include_directories(millionaire PRIVATE ${Project_INSTALL_PREFIX}/include)
target_link_libraries(millionaire PRIVATE  ${EXTERNALLIBS} gmp gmpxx rt OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
target_link_libraries(millionaire PRIVATE fmt::fmt-header-only nlohmann_json)
set_property(TARGET millionaire PROPERTY CXX_STANDARD 14)
