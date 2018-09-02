#!/bin/sh

set -e

SOURCE_PATH="$1"

echo "Patching valijson: $SOURCE_PATH"

find "$SOURCE_PATH" -type f \( -name "nlohmann*.hpp" \) \
    -exec sed -i -e 's@^#include <json.hpp@#include <nlohmann/json.hpp@' {} \;

if ! grep -q valijson_INSTALL_HEADERS "${SOURCE_PATH}/CMakeLists.txt"; then
  sed -i -E -e 's/(INSTALL_HEADERS|BUILD_EXAMPLES|BUILD_TESTS)/valijson_\1/g' \
    "${SOURCE_PATH}/CMakeLists.txt"
fi

touch "$SOURCE_PATH/patched"
