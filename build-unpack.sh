#!/bin/bash
# Build aros/unpack/FGUnpack.cpp (unpacks FlightGear's data archive on AROS) against the SimGear in work/prefix
# into out/FGUnpack. Linked as fgfs is: static, -nix.
. "$(dirname "$0")/env.sh"
CXX=$BUILD/bin/linux-x86_64/tools/crosstools/x86_64-aros-g++
mkdir -p $P/out $P/logs
$CXX -O2 -std=c++17 $FGAROS_FLAGS -I$PREFIX/include -nix -o $P/out/FGUnpack $P/aros/unpack/FGUnpack.cpp \
    -L$PREFIX/lib -lSimGearCore -lcurl -lFGExpat $PREFIX/lib/liblzma.a -lFGUdns -lssl -lcrypto -lzstd -lbrotlidec \
    -lbrotlicommon $SDK/lib/libz.static.a -latomic -lfgaros -lpthread > $P/logs/unpack.log 2>&1
echo "FGUnpack rc $?"
