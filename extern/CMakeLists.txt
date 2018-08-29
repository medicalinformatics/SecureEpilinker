message(STATUS "Building external projects")

add_subdirectory(ABY)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
add_subdirectory(json EXCLUDE_FROM_ALL)
add_subdirectory(cxxopts EXCLUDE_FROM_ALL)
add_subdirectory(valijson EXCLUDE_FROM_ALL)

add_subdirectory(restbed)

message(STATUS "Patching valijson")
add_custom_command(OUTPUT valijson_patched COMMAND
  "${PROJECT_SOURCE_DIR}/cmake/patch-valijson.sh"
  "${PROJECT_SOURCE_DIR}/extern/valijson")
add_custom_target(valijson_patch ALL DEPENDS valijson_patched)

add_subdirectory(curlpp)
