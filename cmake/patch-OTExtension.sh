#!/bin/sh

set -e

SOURCE_PATH="$1"

echo "Patching OTExtension source: $SOURCE_PATH"
find "$SOURCE_PATH" -type f \( -name "*.h" -o -name "*.cpp" \) \
    -exec sed -i -e 's@^#include "\.\./miracl_lib/@#include "miracl_lib/@' \
                 -e 's@^#include "\.\./ENCRYPTO_utils/@#include "ENCRYPTO_utils/@' {} \;
