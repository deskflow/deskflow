#! /bin/bash

export SYNERGY_NO_LEGACY=1
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
mkdir -p build
pushd build
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
make synergyc synergys
popd
