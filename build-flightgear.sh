#!/bin/bash
# Cross-build FlightGear for AROS (needs build-osg.sh, build-compat.sh, build-simgear.sh and build-plib.sh
# installed into work/prefix). Usage: build-flightgear.sh configure | make [target] | install
# The sources are those of the local branch "aros" in upstream/src/flightgear (aros/patches/flightgear).
# Left out: the Qt launcher, crash reporting, FGCom/IAX, DBus, VR, swift, HID/event input, the test suite and the
# separate tools. Joysticks go through 3rdparty/joystick/jsAROS.cxx (controller.hidd).
. "$(dirname "$0")/env.sh"
W=$P/work/build-flightgear
mkdir -p $W $PREFIX $P/logs
# What the link needs beyond FlightGear's own list: OpenSceneGraph's plugins and serializers, which a static build
# registers in fg_os_osgviewer.cxx, with the libraries they use, and what the static libcurl uses. zlib is the static
# one and FreeType the static one: the stub libraries of z.library and freetype2.library lack gzgetc() and
# FT_Outline_Get_Orientation().
PLUGINS=$(cd $PREFIX/lib/osgPlugins-3.6.5 && ls libosgdb_*.a | sed 's/^lib/-l/; s/\.a$//' | tr '\n' ' ')
LINKLIBS="-L$PREFIX/lib -L$PREFIX/lib/osgPlugins-3.6.5 -Wl,--start-group $PLUGINS -losgTerrain -losgFX -losgSim"
LINKLIBS="$LINKLIBS -losgParticle -losgText -losgViewer -losgGA -losgUtil -losgDB -losg -lOpenThreads -Wl,--end-group"
LINKLIBS="$LINKLIBS $SDK/lib/libfreetype2.static.a -lpng -ljpeg -lssl -lcrypto -lzstd -lbrotlidec -lbrotlicommon $SDK/lib/libz.static.a"
LINKLIBS="$LINKLIBS -latomic -lfgaros -lpthread"
# -nix: Unix path syntax in the C library (/PROGDIR/fgdata is PROGDIR:fgdata), which is what SGPath takes for an
# absolute path.
case "$1" in
configure)
    cmake -S $SRC/flightgear -B $W -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$PREFIX \
        -DPKG_CONFIG_EXECUTABLE=$BUILD/bin/linux-x86_64/tools/x86_64-aros-pkg-config \
        -DOpenGL_GL_PREFERENCE=LEGACY -DOPENGL_INCLUDE_DIR=$SDK/include \
        -DOPENGL_gl_LIBRARY=$SDK/lib/libGL.a -DOPENGL_glu_LIBRARY=$SDK/lib/libGLU.a \
        -DOPENAL_INCLUDE_DIR=$SDK/include/AL -DOPENAL_LIBRARY=$SDK/lib/libopenal.a \
        -DCURL_INCLUDE_DIR=$SDK/include -DCURL_LIBRARY=$SDK/lib/libcurl.a \
        -DZLIB_INCLUDE_DIR=$SDK/include -DZLIB_LIBRARY=$SDK/lib/libz.static.a \
        -DLIBLZMA_INCLUDE_DIR=$PREFIX/include -DLIBLZMA_LIBRARY=$PREFIX/lib/liblzma.a \
        -DBoost_INCLUDE_DIR=$SDK/include \
        -DCMAKE_C_FLAGS="$FGAROS_FLAGS" \
        -DCMAKE_CXX_FLAGS="-DOSG_LIBRARY_STATIC $FGAROS_FLAGS" \
        -DCMAKE_EXE_LINKER_FLAGS="-nix" \
        -DCMAKE_C_STANDARD_LIBRARIES="$LINKLIBS" \
        -DCMAKE_CXX_STANDARD_LIBRARIES="$LINKLIBS" \
        -DFGRCC_EXECUTABLE=$P/work/host/bin/fgrcc -DFG_DATA_DIR="/PROGDIR/fgdata" -DOSG_PLUGINS_DIR=$PREFIX/lib/osgPlugins-3.6.5 \
        -DENABLE_QT=OFF -DENABLE_SENTRY=OFF -DENABLE_IAX=OFF -DENABLE_FGCOM=OFF -DUSE_DBUS=OFF \
        -DENABLE_VR=OFF -DENABLE_SWIFT=OFF -DEVENT_INPUT=OFF -DENABLE_HID_INPUT=OFF \
        -DENABLE_PLIB_JOYSTICK=ON -DENABLE_RTI=OFF -DENABLE_AUTOTESTING=OFF \
        -DENABLE_FGELEV=OFF -DENABLE_FGVIEWER=OFF -DENABLE_GPSSMOOTH=OFF -DENABLE_FGJS=OFF \
        -DENABLE_JS_DEMO=OFF -DENABLE_METAR=OFF -DENABLE_TRAFFIC=OFF -DENABLE_DEMCONVERT=OFF \
        -DENABLE_STGMERGE=OFF -DWITH_FGPANEL=OFF \
        > $P/logs/flightgear-configure.log 2>&1
    echo "configure rc $?" ;;
make)
    cmake --build $W -j32 ${2:+--target $2} -- -k 0 > $P/logs/flightgear-make.log 2>&1
    echo "make rc $?" ;;
install)
    cmake --install $W > $P/logs/flightgear-install.log 2>&1
    echo "install rc $?" ;;
*)  echo "usage: $0 configure|make [target]|install"; exit 2 ;;
esac
