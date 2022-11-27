#!/usr/bin/env bash

## https://stackoverflow.com/questions/4023830/how-to-compare-two-strings-in-dot-separated-version-format-in-bash
checkVer() {
    [  "$1" = "`echo -e "$1\n$2" | sort -V | head -n1`" ]
}

declare -a requirements=("gcc" "cmake")
declare -a versions=( 6.1.0 3.5.0)

for i in ${!requirements[@]};do
  hash ${requirements[$i]} 2>/dev/null || { echo >&2 "[ERROR]: ${requirements[$i]} was not found in path, make sure it is installed correctly"; exit 1; }
  matches=$(${requirements[$i]} --version | grep -E --only-match  "([0-9]+\.?)+")
  #echo ${matches}
  matchesArray=($(echo $matches | tr " " "\n"))
  installedVer=${matchesArray[0]}

  checkVer ${versions[$i]} $installedVer || { echo >&2 "[ERROR]: Your ${requirements[$i]} (${installedVer}) is outdated please use version ${versions[$i]} or higher"; exit 1; }
    
done

echo "[INFO]: All dependencies are met!"
cmake -B build -DCMAKE_BUILD_TYPE=Debug -S Swirl &>/dev/null
echo "[INFO]: Building Swirl"
cmake --build build --config Debug &>/dev/null
echo "Swirl is now available at ${pwd}/Swirl/build/swirl"
./build/swirl -h