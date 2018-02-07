#!/bin/sh
cd "$(dirname $0)" || exit 1
rm -rf build
mkdir build || exit 1
cd build || exit 1
# some environments have cmake v2 as 'cmake' and v3 as 'cmake3'
# check for cmake3 first then fallback to just cmake
CMAKE=`which cmake3 2>/dev/null`
[ $? -ne 0 -o "x$CMAKE" = "x" ] && CMAKE=cmake
$CMAKE -D CMAKE_BUILD_TYPE=Debug .. || exit 1
make || exit 1
echo "Build completed successfully"
