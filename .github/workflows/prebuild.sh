#!/bin/bash
set -x -e 

mkdir -p be/install && cd be

# Build METIS
git clone https://github.com/KarypisLab/METIS.git METIS
cmake \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=./install \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -B METIS/build \
    METIS

cmake --build METIS/build --target install $MAKEFLAGS

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