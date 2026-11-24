#!/bin/bash
# Cross-build the parts of PLIB FlightGear's PUI user interface needs (ul, sg, fnt, pu, puaux) for AROS as
# static libraries into work/prefix.
# Usage: build-plib.sh configure | make [target] | install
. "$(dirname "$0")/env.sh"
W=$P/work/build-plib
mkdir -p $W $PREFIX $P/logs
case "$1" in
configure)
    cmake -S $SRC/plib -B $W -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_INSTALL_LIBDIR=$PREFIX/lib \
        -DOpenGL_GL_PREFERENCE=LEGACY -DOPENGL_INCLUDE_DIR=$SDK/include \
        -DOPENGL_gl_LIBRARY=$SDK/lib/libGL.a -DOPENGL_glu_LIBRARY=$SDK/lib/libGLU.a \
        -DENABLE_FNT=ON -DENABLE_PUAUX=ON -DENABLE_PUI=ON -DENABLE_SG=ON -DENABLE_UTIL=ON \
        -DENABLE_JS=OFF -DENABLE_NET=OFF -DENABLE_PSL=OFF -DENABLE_PW=OFF -DENABLE_SL=OFF \
        -DENABLE_SSG=OFF -DENABLE_SSGAUX=OFF \
        -DCMAKE_C_FLAGS="$PATHMAP_FLAGS" -DCMAKE_CXX_FLAGS="$PATHMAP_FLAGS" \
        > $P/logs/plib-configure.log 2>&1
    echo "configure rc $?" ;;
make)
    cmake --build $W -j32 ${2:+--target $2} -- -k 0 > $P/logs/plib-make.log 2>&1
    echo "make rc $?" ;;
install)
    cmake --install $W > $P/logs/plib-install.log 2>&1
    echo "install rc $?" ;;
*)  echo "usage: $0 configure|make [target]|install"; exit 2 ;;
esac
