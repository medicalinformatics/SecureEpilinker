configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/MIRACL.CMakeLists.txt.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MIRACL.CMakeLists.txt"
    @ONLY
)

ExternalProject_Add(MIRACL
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/extern/MIRACL"
    INSTALL_DIR "${SecureEpiLinker_INSTALL_PREFIX}"
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/MIRACL.CMakeLists.txt"
        "<TMP_DIR>/CMakeLists.txt"
    COMMAND find "<SOURCE_DIR>/" -type d -name ".git" -prune -o -type f -exec cp --preserve=timestamps {} "<TMP_DIR>/" "$<SEMICOLON>"
    COMMAND cmake -E copy_if_different "<TMP_DIR>/mirdef.h64" "<TMP_DIR>/mirdef.h"
    COMMAND cmake -E copy_if_different "<TMP_DIR>/mrmuldv.g64" "<TMP_DIR>/mrmuldv.c"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cmake
        -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_CXX_COMPILER:PATH=${CMAKE_CXX_COMPILER}
        -DCMAKE_C_COMPILER:PATH=${CMAKE_C_COMPILER}
        "<TMP_DIR>"
)

