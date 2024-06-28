#!/bin/bash

cmake_format='build/python/bin/cmake-format'
match='-name CMakeLists.txt -o -name *.cmake'
exclude='-path ./ext -o -path ./build'

# Set check argument based on --format input
args="--check"
if [ "$1" == "--format" ]; then
  args="--in-place"
fi

files=$(find . \( $exclude \) -prune -o \( $match \) -print)
if [ -n "$files" ]; then
  echo "$files" | xargs $cmake_format $args
else
  echo "No CMake files found to lint."
fi
