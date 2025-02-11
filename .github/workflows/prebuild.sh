#!/bin/bash
set -x -e 

mkdir -p be/install && cd be

# Install METIS
if [[ $1 == "macos-13" ||  $1 == "macos-14" ]]; then
  brew install metis
elif [[ $1 == "ubuntu-20.04" ]]; then
  yum install -y metis metis-devel ninja-build
fi

# Build ITK
git clone -b v5.4.0 https://github.com/InsightSoftwareConsortium/ITK.git ITK
cmake \
    -DModule_MorphologicalContourInterpolation=ON \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=./install \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -B ITK/build \
    ITK

cmake --build ITK/build --target install $MAKEFLAGS