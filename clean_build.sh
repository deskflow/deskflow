#!/bin/sh
cd "$(dirname $0)" || exit 1
rm -rf build
mkdir build || exit 1
cd build || exit 1
# some environments have cmake v2 as 'cmake' and v3 as 'cmake3'
# check for cmake3 first then fallback to just cmake
B_CMAKE=`which cmake3 2>/dev/null`
[ $? -ne 0 -o "x$B_CMAKE" = "x" ] && B_CMAKE=cmake
# default build configuration
B_BUILD_TYPE=${B_BUILD_TYPE:-Debug}
if [ "$(uname)" = "Darwin" ]; then
    # OSX needs a lot of extra help, poor thing
    # run the osx_environment.sh script to fix paths
    [ -r ../osx_environment.sh ] && source ../osx_environment.sh
    B_CMAKE_FLAGS="-DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk $B_CMAKE_FLAGS"
fi
B_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=$B_BUILD_TYPE $B_CMAKE_FLAGS"
$B_CMAKE $B_CMAKE_FLAGS .. || exit 1
make || exit 1
echo "Build completed successfully"
