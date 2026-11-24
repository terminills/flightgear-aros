#!/bin/bash
# Cross-build SimGear for AROS as a static library into work/prefix (needs build-osg.sh install and build-deps.sh).
# Usage: build-simgear.sh configure | make [target] | install
# The sources are those of the local branch "aros" in upstream/src/simgear (aros/patches/simgear).
. "$(dirname "$0")/env.sh"
W=$P/work/build-simgear
mkdir -p $W $PREFIX $P/logs
case "$1" in
configure)
    mkdir -p $PREFIX/lib/cmake/Boost && cp $P/aros/cmake/BoostConfig.cmake $PREFIX/lib/cmake/Boost/ || exit 1
    cmake -S $SRC/simgear -B $W -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$PREFIX \
        -DPKG_CONFIG_EXECUTABLE=$BUILD/bin/linux-x86_64/tools/x86_64-aros-pkg-config \
        -DOpenGL_GL_PREFERENCE=LEGACY -DOPENGL_INCLUDE_DIR=$SDK/include \
        -DOPENGL_gl_LIBRARY=$SDK/lib/libGL.a -DOPENGL_glu_LIBRARY=$SDK/lib/libGLU.a \
        -DOPENAL_INCLUDE_DIR=$SDK/include/AL -DOPENAL_LIBRARY=$SDK/lib/libopenal.a \
        -DCURL_INCLUDE_DIR=$SDK/include -DCURL_LIBRARY=$SDK/lib/libcurl.a \
        -DZLIB_INCLUDE_DIR=$SDK/include -DZLIB_LIBRARY=$SDK/lib/libz.a \
        -DLIBLZMA_INCLUDE_DIR=$PREFIX/include -DLIBLZMA_LIBRARY=$PREFIX/lib/liblzma.a \
        -DBoost_INCLUDE_DIR=$SDK/include \
        -DCMAKE_C_FLAGS="$FGAROS_FLAGS" \
        -DCMAKE_CXX_FLAGS="-DOSG_LIBRARY_STATIC $FGAROS_FLAGS" -DHAVE_TIMEGM=1 \
        -DSIMGEAR_SHARED=OFF -DENABLE_TESTS=OFF -DENABLE_PKGUTIL=OFF -DENABLE_CYCLONE=OFF \
        -DUSE_AEONWAVE=OFF -DENABLE_VIDEO_RECORD=OFF -DENABLE_GDAL=OFF -DENABLE_RTI=OFF \
        > $P/logs/simgear-configure.log 2>&1
    echo "configure rc $?" ;;
make)
    cmake --build $W -j32 ${2:+--target $2} -- -k 0 > $P/logs/simgear-make.log 2>&1
    echo "make rc $?" ;;
install)
    cmake --install $W > $P/logs/simgear-install.log 2>&1
    echo "install rc $?" ;;
*)  echo "usage: $0 configure|make [target]|install"; exit 2 ;;
esac
