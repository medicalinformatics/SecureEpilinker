#!/bin/sh

echo Switching to repo toplevel directory
set -x -e
cd $(git rev-parse --show-toplevel)

git submodule update --init

cd extern/restbed
git submodule update --init dependency/{asio,catch} # ignore openssl

cd ../ABY
git submodule update --init # ENCRYPTO_utils and OTExtension
cd extern/ENCRYPTO_utils
git submodule update --init # MIRACL
