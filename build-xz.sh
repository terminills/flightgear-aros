#!/bin/bash
# Cross-build liblzma (xz, decoders only) for AROS as a static library into work/prefix. SimGear unpacks .txz
# archives with it. The lzma.library AROS comes with is not used: it is built without any filter and without
# any integrity check, so that it reads no .xz file (tests/xzcheck.c shows both).
# Usage: build-xz.sh configure | make [target] | install
. "$(dirname "$0")/env.sh"
W=$P/work/build-xz
mkdir -p $W $PREFIX $P/logs
case "$1" in
configure)
    cmake -S $SRC/xz -B $W -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_INSTALL_LIBDIR=$PREFIX/lib \
        -DBUILD_SHARED_LIBS=OFF -DXZ_THREADS=no -DXZ_NLS=OFF -DXZ_DOC=OFF -DXZ_SANDBOX=no \
        -DXZ_ENCODERS= -DXZ_MATCH_FINDERS= -DXZ_MICROLZMA_ENCODER=OFF -DXZ_CLMUL_CRC=OFF \
        -DXZ_TOOL_XZ=OFF -DXZ_TOOL_XZDEC=OFF -DXZ_TOOL_LZMADEC=OFF -DXZ_TOOL_LZMAINFO=OFF -DXZ_TOOL_SCRIPTS=OFF \
        -DCMAKE_C_FLAGS="$PATHMAP_FLAGS" \
        > $P/logs/xz-configure.log 2>&1
    echo "configure rc $?" ;;
make)
    cmake --build $W -j32 ${2:+--target $2} -- -k 0 > $P/logs/xz-make.log 2>&1
    echo "make rc $?" ;;
install)
    cmake --install $W > $P/logs/xz-install.log 2>&1
    echo "install rc $?" ;;
*)  echo "usage: $0 configure|make [target]|install"; exit 2 ;;
esac
