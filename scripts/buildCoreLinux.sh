#! /bin/zsh

mkdir -p build
pushd build
cmake ..
make synergyc synergys
popd
