#!/bin/bash

set -eu # we set this to catch errors and terminate

cd "$(dirname "$0")" || exit 1

# some environments have cmake v2 as 'cmake' and v3 as 'cmake3'
# check for cmake3 first then fallback to just cmake

if type cmake3 2>/dev/null; then
    B_CMAKE=$(command -v "$(echo "$B_CMAKE" | cut -d' ' -f3)")
else
    B_CMAKE=$(command -v cmake)
fi

# default build configuration
B_BUILD_TYPE=${B_BUILD_TYPE:-Debug}

if [ "$(uname -s)" = "Darwin" ]; then
    # OSX needs a lot of extra help, poor thing
    # run the osx_environment.sh script to fix paths
    if [ -f "./osx_environment.sh" ]; then
       . ./osx_environment.sh
    fi
    B_CMAKE_FLAGS="-DCMAKE_OSX_SYSROOT=$(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 $B_CMAKE_FLAGS"
fi

# allow local customizations to build environment
if [ -f "./build_env.sh" ]; then
    . ./build_env.sh
fi

set +eu # disable this temporarily
if [ -n "${B_CMAKE_FLAGS}" ]; then
    B_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=$B_BUILD_TYPE ${B_CMAKE_FLAGS}"
else
    B_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=$B_BUILD_TYPE"
fi
set -eu # enable this

rm -rf build
mkdir build || exit 1
cd build || exit 1

echo "Starting Barrier build..."
echo "Build type: ${B_BUILD_TYPE}"

"$B_CMAKE" "$B_CMAKE_FLAGS" .. || exit 1

make || exit 1

echo "Build completed successfully."
