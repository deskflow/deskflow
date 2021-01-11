#! /bin/bash

export SYNERGY_NO_LEGACY=1
mkdir -p build
pushd build
cmake ..
make synergyc synergys
popd
