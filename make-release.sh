#!/bin/bash
# make-release.sh TAG (for example rc1): the release archives in out/release/
#   FlightGear-<version>-AROS-<TAG>-x86_64.lha (stored, tools/mklha.py) and .zip: the drawer FlightGear with fgfs,
#     FGUnpack, Libs/openal.library, Drivers/llvmpipe.hidd with the script UseLLVMpipe, the options file .fgfsrc,
#     ReadMe and the licences;
#   FlightGear-<version>-AROS-<TAG>-source.tar.xz: the sources fgfs was built from (upstream/src/* at their "aros"
#     branch, which must have nothing uncommitted) and this port's own files;
#   SHA256SUMS.
# The programs are what rebuild-all.sh, build-unpack.sh and the AROS build tree (BUILD in env.sh) made last. They are
# stripped as AROS strips its own, the build machine's directories are taken out (tools/scrub-paths.py), and the
# script stops if any file of the drawer still names one or if fgfs carries a relocation AROS cannot load.
. "$(dirname "$0")/env.sh"
TAG=${1:?usage: make-release.sh TAG}
V=$(cat $SRC/flightgear/flightgear-version) || exit 1
N=FlightGear-$V-AROS-$TAG
R=$P/out/release
D=$R/FlightGear
STRIP="$BUILD/bin/linux-x86_64/tools/x86_64-pc-aros-strip --strip-unneeded -R.comment"
AROS_SRC=$(sed -n 's/^SRCDIR *:\{0,1\}= *//p' $BUILD/Makefile | head -1)
for d in flightgear simgear openscenegraph plib xz; do
    [ -z "$(git -C $SRC/$d status --short)" ] || { echo "$d has uncommitted changes"; exit 1; }
done
[ -z "$(git -C $P status --short)" ] || { echo "the port repository has uncommitted changes"; exit 1; }
rm -rf $R && mkdir -p $D/Libs $D/Drivers $D/Licences || exit 1
$STRIP -o $D/fgfs $P/work/build-flightgear/src/Main/fgfs || exit 1
$STRIP -o $D/FGUnpack $P/out/FGUnpack || exit 1
$STRIP -o $D/Libs/openal.library $BUILD/bin/pc-x86_64/AROS/Libs/openal.library || exit 1
# the faster of AROS's two software GL drivers; AROS master builds it with its LLVM toolchain only, the build tree
# here has it through aros/local-build (build-deps.sh)
$STRIP -o $D/Drivers/llvmpipe.hidd $BUILD/bin/pc-x86_64/AROS/Devs/Drivers/llvmpipe.hidd || exit 1
python3 $P/tools/scrub-paths.py --map $BUILD=/aros-build --map $AROS_SRC=/aros-src --map $P=/port $D/fgfs $D/FGUnpack $D/Libs/openal.library $D/Drivers/llvmpipe.hidd || exit 1
python3 $P/tools/elf-relocs.py $D/fgfs $D/FGUnpack || exit 1
cp $P/aros/release/ReadMe $D/ReadMe && cp $P/aros/release/fgfsrc $D/.fgfsrc &&
    cp $P/aros/release/UseLLVMpipe $D/UseLLVMpipe || exit 1
cp $SRC/flightgear/COPYING $D/Licences/FlightGear-COPYING &&
    cp $SRC/simgear/COPYING $D/Licences/SimGear-COPYING &&
    cp $SRC/openscenegraph/LICENSE.txt $D/Licences/OpenSceneGraph-LICENSE.txt &&
    cp $SRC/plib/COPYING $D/Licences/PLIB-COPYING &&
    cp $P/aros/release/LICENSE $D/Licences/AROS-port-GPL-2.0 || exit 1
# the libraries FlightGear and SimGear carry in their own trees and link in
cp $SRC/simgear/3rdparty/expat/Expat.COPYING $D/Licences/Expat-COPYING &&
    cp $SRC/simgear/3rdparty/udns/COPYING.LGPL $D/Licences/udns-COPYING.LGPL &&
    cp $SRC/flightgear/3rdparty/cjson/LICENSE $D/Licences/cJSON-LICENSE &&
    cp $SRC/flightgear/3rdparty/mongoose/LICENSE $D/Licences/mongoose-LICENSE &&
    cp $SRC/flightgear/3rdparty/flite_hts_engine/COPYING $D/Licences/flite_hts_engine-COPYING &&
    cp $SRC/flightgear/3rdparty/hts_engine_API/COPYING $D/Licences/hts_engine_API-COPYING || exit 1
# the libraries of the AROS build tree that are linked into the programs (and OpenAL, the library beside them)
PORTS=$BUILD/bin/pc-x86_64/Ports
lic() {  # name file-in-Ports...
    n=$1; shift
    for f in "$@"; do
        set -- $PORTS/$f; [ -f "$1" ] || { echo "no licence file $f"; exit 1; }
        cp "$1" $D/Licences/$n-$(basename "$1") || exit 1
    done
}
lic curl curl/curl-*/COPYING
lic OpenSSL openssl/openssl-*/LICENSE.txt
lic OpenAL-Soft openal-soft/openal-soft-*/COPYING
lic zlib zlib/zlib/LICENSE
cp $SRC/xz/COPYING $D/Licences/xz-COPYING && cp $SRC/xz/COPYING.0BSD $D/Licences/xz-COPYING.0BSD || exit 1
lic zstd zstd/zstd-*/LICENSE
lic brotli brotli/brotli-1.2*/LICENSE
lic FreeType freetype2/freetype-*/LICENSE.TXT freetype2/freetype-*/docs/FTL.TXT
lic libpng libpng/libpng-*/LICENSE
lic libjpeg jpeg/jpeg-*/README
lic Mesa mesa/mesa-*/docs/license.rst
lic LLVM llvm/llvm-*.src/LICENSE.TXT
cp $AROS_SRC/LICENSE $D/Licences/AROS-LICENSE || exit 1
cp $BUILD/bin/linux-x86_64/Ports/host/gcc/gcc-*/COPYING.RUNTIME $D/Licences/libstdc++-COPYING.RUNTIME || exit 1
# a text with a figure still to be measured is a draft: DRAFT=1 packs it for a test, nothing else does
n=$(command grep -c '@TODO' $D/ReadMe $P/aros/release/README.md | awk -F: '{s += $2} END {print s}')
[ "$n" = 0 ] || [ "$DRAFT" = 1 ] || { echo "ReadMe or README.md: $n places still to be filled in"; exit 1; }
bad=0
for f in $(find $D -type f); do
    n=$(strings -a $f | command grep -c '/hom[e]/\|/ru[n]/media')
    [ "$n" = 0 ] || { echo "$f: $n strings with a build machine directory"; bad=1; }
done
[ $bad = 0 ] || exit 1
(cd $R && python3 $P/tools/mklha.py $N-x86_64.lha FlightGear && zip -qr $N-x86_64.zip FlightGear) || exit 1
# the sources: each tree under its own name, the port's files (as git has them) under flightgear-aros
T=$R/src.tmp; mkdir -p $T/$N-source
for d in flightgear simgear openscenegraph plib xz; do
    git -C $SRC/$d archive --prefix=$d/ HEAD | tar -x -C $T/$N-source || exit 1
done
$P/export-public.sh $T/$N-source/flightgear-aros || exit 1
tar -C $T --sort=name --owner=0 --group=0 --numeric-owner -cf - $N-source | xz -T0 -6 > $R/$N-source.tar.xz || exit 1
rm -rf $T
(cd $R && sha256sum $N-x86_64.lha $N-x86_64.zip $N-source.tar.xz > SHA256SUMS && cat SHA256SUMS)
(cd $D && find . -type f | sort | xargs sha256sum) > $R/drawer.sha256
ls -l $R/$N-*
