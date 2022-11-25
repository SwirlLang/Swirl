#!/usr/bin/env bash

declare -a requirements=("gcc" "cmake")

for req in ${requirements[@]};do
  hash $req 2>/dev/null || { echo >&2 "[ERROR]: ${req} was not found in path, make sure it is installed correctly"; exit 1; }
done

echo "[INFO]: All dependencies are met!"
cmake -B build -DCMAKE_BUILD_TYPE=Debug -S Swirl &>/dev/null
echo "[INFO]: Building Swirl"
cmake --build build --config Debug >/dev/null
echo "Swirl is now available at ${pwd}/Swirl/build/swirl"
./build/swirl -h