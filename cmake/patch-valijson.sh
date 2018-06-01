#!/bin/sh

set -e

SOURCE_PATH="$1"

echo "Patching valijson source: $SOURCE_PATH"
find "$SOURCE_PATH" -type f \( -name "nlohmann*.hpp" \) \
    -exec sed -i -e 's@^#include <json.hpp@#include <nlohmann/json.hpp@' {} \;
touch "$SOURCE_PATH/patched"
