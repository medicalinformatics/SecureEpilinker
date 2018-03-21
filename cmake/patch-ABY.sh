#!/bin/sh

set -e

SOURCE_PATH="$1"

echo "Patching ABY source: $SOURCE_PATH"
find "$SOURCE_PATH" -type f \( -name "*.h" -o -name "*.cpp" \) \
    -exec sed -i -e 's@^#include "\.\./miracl_lib/@#include "miracl_lib/@' \
                 -e 's@^#include "\.\./ENCRYPTO_utils/@#include "ENCRYPTO_utils/@' \
                 -e '/arithmtmasking.h/! s@^#include "\.\./ot/@#include "ot/@' \
                 -e 's@^#include "\(\.\./\)\+abycore/ENCRYPTO_utils/@#include "ENCRYPTO_utils/@' {} \;

sed -i -e 's@^#include "maskingfunction.h"$@#include "ot/maskingfunction.h"@' \
    "$SOURCE_PATH/src/abycore/ot/arithmtmasking.h"
