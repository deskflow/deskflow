#!/bin/sh

if [ ! $BARRIER_BUILD_ENV ]; then

    printf "Modifying environment for Barrier build..."

    QT_PATH=$(brew --prefix qt)
    OPENSSL_PATH=$(brew --prefix openssl)

    export CMAKE_PREFIX_PATH="$QT_PATH:$CMAKE_PREFIX_PATH"
    export LD_LIBRARY_PATH="$OPENSSL_PATH/lib:$LD_LIBRARY_PATH"
    export CPATH="$OPENSSL_PATH/include:$CPATH"
    export PKG_CONFIG_PATH="$OPENSSL_PATH/lib/pkgconfig:$PKG_CONFIG_PATH"
    export BARRIER_BUILD_ENV=1

    printf "done\n"
fi
