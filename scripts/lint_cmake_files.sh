#!/bin/bash

cmake_format='build/python/bin/cmake-format'
match='-name CMakeLists.txt -o -name *.cmake'
exclude='-path ./ext -o -path ./build'

files=$(find . \( $exclude \) -prune -o \( $match \) -print)
if [ -n "$files" ]; then
  echo "$files" | xargs $cmake_format --check
else
  echo "No CMake files found to lint."
fi
