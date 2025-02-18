#!/usr/bin/env bash

set -exuo pipefail

# Build geoarrow statically.
cmake -S ../.. -B scratch/geoarrow_build_static/ \
    -DCMAKE_INSTALL_PREFIX=scratch/geoarrow_install_static/
cmake --build scratch/geoarrow_build_static/
cmake --install scratch/geoarrow_build_static/

# Build geoarrow dynamically.
cmake -S ../.. -B scratch/geoarrow_build_shared/ \
    -DCMAKE_INSTALL_PREFIX=scratch/geoarrow_install_shared/ \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
cmake --build scratch/geoarrow_build_shared/
cmake --install scratch/geoarrow_build_shared/

for geoarrow_build_type in static shared; do
    # Build the project against the built geoarrow.
    cmake -S . -B scratch/build_${geoarrow_build_type}/ -DCMAKE_PREFIX_PATH=scratch/geoarrow_build_${geoarrow_build_type}/
    cmake --build scratch/build_${geoarrow_build_type}/

    # Build the project against the installed geoarrow.
    cmake -S . -B scratch/build_against_install_${geoarrow_build_type}/ -DCMAKE_PREFIX_PATH=scratch/geoarrow_install_${geoarrow_build_type}/
    cmake --build scratch/build_against_install_${geoarrow_build_type}/

    # Now try using FetchContent to get geoarrow from remote.
    cmake -S . -B scratch/build_against_fetched_${geoarrow_build_type}/ -DFIND_GEOARROW=OFF
    cmake --build scratch/build_against_fetched_${geoarrow_build_type}/
done
