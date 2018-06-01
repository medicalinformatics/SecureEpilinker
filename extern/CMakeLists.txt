message(STATUS "Building external projects")

include(ExternalProject)

include(External_MIRACL)
include(External_ENCRYPTO_utils)
include(External_OTExtension)
include(External_ABY)

add_dependencies(ENCRYPTO_utils MIRACL)
add_dependencies(OTExtension ENCRYPTO_utils MIRACL)
add_dependencies(ABY OTExtension ENCRYPTO_utils MIRACL)

add_subdirectory(fmt EXCLUDE_FROM_ALL)
add_subdirectory(json EXCLUDE_FROM_ALL)
add_subdirectory(valijson EXCLUDE_FROM_ALL)

message(STATUS "Patching valijson")
add_custom_command(OUTPUT patched COMMAND
  "${PROJECT_SOURCE_DIR}/cmake/patch-valijson.sh"
  "${PROJECT_SOURCE_DIR}/extern/valijson")
add_custom_target(valijson_patch ALL DEPENDS patched)
