#/bin/bash

mkdir build
cmake -S . -B build
cmake --build build -j16

# package

