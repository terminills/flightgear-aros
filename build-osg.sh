#!/bin/bash
# Cross-build FlightGear's OpenSceneGraph fork for AROS as static libraries into work/prefix.
# Usage: build-osg.sh configure | make [target] | install
. "$(dirname "$0")/env.sh"
W=$P/work/build-osg
mkdir -p $W $PREFIX $P/logs
case "$1" in
configure)
    cmake -S $SRC/openscenegraph -B $W -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_PREFIX_PATH="/" \
        -DOpenGL_GL_PREFERENCE=LEGACY -DOPENGL_INCLUDE_DIR=$SDK/include \
        -DOPENGL_gl_LIBRARY=$SDK/lib/libGL.a -DOPENGL_glu_LIBRARY=$SDK/lib/libGLU.a \
        -DFREETYPE_LIBRARY=$SDK/lib/libfreetype2.a -DFREETYPE_INCLUDE_DIRS=$SDK/include \
        -DCMAKE_FIND_ROOT_PATH="$SDK;$PREFIX" \
        -DPKG_CONFIG_EXECUTABLE=$BUILD/bin/linux-x86_64/tools/x86_64-aros-pkg-config \
        -DDYNAMIC_OPENTHREADS=OFF -DDYNAMIC_OPENSCENEGRAPH=OFF \
        -DOSG_WINDOWING_SYSTEM=${OSG_WS:-AROS} -DOPENGL_PROFILE=GL2 \
        -DBUILD_OSG_APPLICATIONS=OFF -DBUILD_OSG_EXAMPLES=OFF -DBUILD_OSG_DEPRECATED_SERIALIZERS=OFF \
        -DCMAKE_DISABLE_FIND_PACKAGE_TIFF=ON -DOSG_USE_LOCAL_LUA_SOURCE=OFF -DOSG_TEXT_USE_FONTCONFIG=OFF \
        -D_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS_EXITCODE=0 \
        -D_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS=1 \
        -DCMAKE_C_FLAGS="$PATHMAP_FLAGS" -DCMAKE_CXX_FLAGS="$PATHMAP_FLAGS" \
        > $P/logs/osg-configure.log 2>&1
    echo "configure rc $?" ;;
make)
    cmake --build $W -j32 ${2:+--target $2} > $P/logs/osg-make.log 2>&1
    echo "make rc $?" ;;
install)
    cmake --install $W > $P/logs/osg-install.log 2>&1
    echo "install rc $?" ;;
*)  echo "usage: $0 configure|make [target]|install"; exit 2 ;;
esac
