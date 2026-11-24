# Common settings for the FlightGear-for-AROS build scripts (sourced).
P=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# BUILD: the AROS build tree (pc-x86_64, configured with --with-gcc-version=15.2.0) whose SDK and cross tools the
# port is built with. From the environment, or from local.env beside this file (not in git).
[ -f $P/local.env ] && . $P/local.env
: ${BUILD:?set BUILD to the AROS build tree, in the environment or in local.env}
SDK=$BUILD/bin/pc-x86_64/AROS/Developer
# aros/cmake/toolchain.cmake finds the build tree through this link (CMake runs again from ninja, without this
# file's variables).
mkdir -p $P/work && ln -sfn $BUILD $P/work/aros-build
TOOLCHAIN=$P/aros/cmake/toolchain.cmake
PREFIX=$P/work/prefix
SRC=$P/upstream/src
# Flags for everything built on top of aros/compat (build-compat.sh): fgaros.h is included into every file, the
# headers in include/fgaros stand in front of the SDK's socket headers, and the SDK's function-like macros for
# bsdsocket.library (socket(), bind() ...) stay off. Link with -lfgaros ahead of the C library.
# The build machine's directories stay out of what is built: __FILE__ and the debug information name a source
# file from the port's directory (upstream/src/..., work/prefix/...) and the AROS build tree as "aros-build".
PATHMAP_FLAGS="-ffile-prefix-map=$P/= -ffile-prefix-map=$BUILD/=aros-build/"
FGAROS_FLAGS="-DBSDSOCKET_NOLIBDEFINES -isystem $PREFIX/include/fgaros -include $PREFIX/include/fgaros.h $PATHMAP_FLAGS"
