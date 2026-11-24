#!/bin/bash
# Build the libraries FlightGear needs that upstream AROS already carries, in the master build tree:
# curl and OpenAL from contrib, and llvmpipe.hidd with the target-side LLVM it links.
# Usage: build-deps.sh [metatarget ...]   (default: all; llvmpipe needs the makefile change of aros/local-build)
. "$(dirname "$0")/env.sh"
export OPT_LLVM=${OPT_LLVM:-20.1.0}
T=${@:-contrib-curl contrib-openal workbench-libs-llvm hidd-llvmpipe}
cd $BUILD || exit 1
for t in $T; do
    echo "=== $t start $(date -Is)"
    make -j32 $t > $P/logs/deps-$t.log 2>&1
    echo "=== $t rc $? $(date -Is)"
done
