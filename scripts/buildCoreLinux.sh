#! /bin/bash

export SYNERGY_NO_LEGACY=1
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
mkdir -p build
pushd build
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
make synergyc synergys
zip synergy-core-macos-x64.zip bin/*
echo "::set-output name=location::build/synergy-core-linux-x64.zip"
echo "::set-output name=name::synergy-core-linux-x64.zip"
popd
