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
