#!/bin/bash
# A CD (volume FGTEST) with the test programs of out/tests and the script the test VM executes.
# Usage: package-test.sh [script in aros/, default run-osgwin] ; LLVMPIPE=1 adds drivers/llvmpipe.hidd from the
# build tree (built there by build-deps.sh), which makes the script choose llvmpipe instead of softpipe;
# FGFS=1 adds the stripped fgfs of work/build-flightgear; FGDATA=1 adds upstream/src/fgdata as the drawer fgdata;
# AHINOVEC=1 adds devs/ahi.device of build-ahi-novec.sh, which the script then puts in front of the ISO's;
# CONTROLLER=1 adds the AROS tree's controller test programs (ControllerSim with virtualpad.hidd, ControllerInfo)
# as the drawer controller, for a simulated joystick; ARC=1 adds contrib's lha and UnZip from the build tree
# (make aminet-util-arc-lha aminet-util-arc-unzip there) as the drawer arc.
# FGARGS="..." replaces the marker @FGARGS@ of the script.
. "$(dirname "$0")/env.sh"
S=${1:-run-osgwin}
C=$P/out/cd
mkdir -p $C/tests || exit 1
for f in $P/out/tests/*; do
    $BUILD/bin/linux-x86_64/tools/x86_64-pc-aros-strip --strip-unneeded -o $C/tests/$(basename $f) $f || exit 1
done
rm -rf $C/fgfs $C/libs
if [ "$FGFS" = 1 ]; then
    # the one library fgfs opens that the base ISO does not carry (contrib-openal, built by build-deps.sh)
    mkdir -p $C/libs && cp $BUILD/bin/pc-x86_64/AROS/Libs/openal.library $C/libs/ || exit 1
    $BUILD/bin/linux-x86_64/tools/x86_64-pc-aros-strip --strip-unneeded -o $C/fgfs \
        $P/work/build-flightgear/src/Main/fgfs || exit 1
fi
sed "s|@FGARGS@|$FGARGS|" $P/aros/$S > $C/rt && cp $P/aros/net-up $P/aros/pair-? $P/aros/twice-pair $P/aros/sgnet-pair $P/aros/same-pair $P/aros/keydef $P/aros/*.xml $P/aros/js-moves $P/aros/*.sim $C/ || exit 1
rm -rf $C/controller
if [ "$CONTROLLER" = 1 ]; then
    mkdir -p $C/controller
    cp $BUILD/bin/pc-x86_64/AROS/Developer/Debug/Tests/controller/{ControllerSim,ControllerInfo,virtualpad.hidd} \
        $C/controller/ || exit 1
fi
rm -f $C/llvmpipe $C/drivers/llvmpipe.hidd; [ ! -d $C/drivers ] || rmdir $C/drivers
if [ "$LLVMPIPE" = 1 ]; then
    mkdir -p $C/drivers
    $BUILD/bin/linux-x86_64/tools/x86_64-pc-aros-strip --strip-unneeded -o $C/drivers/llvmpipe.hidd \
        $BUILD/bin/pc-x86_64/AROS/Devs/Drivers/llvmpipe.hidd || exit 1
fi
rm -rf $C/arc
if [ "$ARC" = 1 ]; then
    mkdir -p $C/arc
    cp $BUILD/bin/pc-x86_64/AROS/Extras/Misc/aminet/C/lha $BUILD/bin/pc-x86_64/AROS/C/UnZip $C/arc/ || exit 1
fi
rm -rf $C/devs
if [ "$AHINOVEC" = 1 ]; then
    mkdir -p $C/devs && cp $P/out/ahi-novec/ahi.device $C/devs/ || exit 1
fi
GRAFT=
[ "$FGDATA" = 1 ] && GRAFT="-m .git -graft-points fgdata=$SRC/fgdata"
xorriso -as mkisofs -R -J -V FGTEST -o $P/out/fgtest-cd.iso $GRAFT $C > $P/out/iso.log 2>&1 || { tail -3 $P/out/iso.log; exit 1; }
echo "$(ls $C/tests | tr '\n' ' '): $(du -sh $C | cut -f1) drawer; $(ls -l $P/out/fgtest-cd.iso | awk '{print $5}') bytes CD"
