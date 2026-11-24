#!/bin/bash
# make-hd.sh new | fgfs | home: the test VM's hard disk (out/fgfs-hd.img): an MBR with one FAT32 partition, volume FG,
# holding fgfs (stripped), libs/openal.library and FGData as the drawer fgdata, so that FG:fgfs finds its data
# (/PROGDIR/fgdata) and keeps its home (/PROGDIR/.fgfs: navigation cache, settings, logs) from run to run.
# "new" makes the disk; "fgfs" only replaces the program on an existing one; "home" also empties FlightGear's
# home (a first start again). No VM may have the disk open.
# make-hd.sh release [FILE...]: a disk as a user's would be (out/rel-hd.img, or $IMG): the drawer FlightGear as
# it comes out of the release archive out/release/*.lha (every file compared with out/release/drawer.sha256)
# and nothing else; with FILEs, those beside the drawer: the data archive for "FGUnpack FG:<name>", the release
# archives themselves for unpacking them on AROS (aros/run-rel-arc).
. "$(dirname "$0")/env.sh"
IMG=${IMG:-$P/out/fgfs-hd.img}
[ "$1" = release ] && IMG=${IMG%fgfs-hd.img}rel-hd.img
SIZE_GB=${SIZE_GB:-8}
START=2048
export MTOOLS_SKIP_CHECK=1
M="-i $IMG@@$((START * 512))"
pgrep -f "file=$IMG" >/dev/null && { echo "a QEMU has $IMG open"; exit 1; }
case "$1" in
new)
    rm -f $IMG && truncate -s ${SIZE_GB}G $IMG || exit 1
    echo "start=$START, type=c" | /usr/sbin/sfdisk -q $IMG || exit 1
    /usr/sbin/mkfs.vfat -F 32 -n FG -h $START --offset $START $IMG $(( (SIZE_GB * 1024 * 1024 * 1024 / 512 - START) / 2 )) \
        > $P/out/hd.log 2>&1 || { tail -3 $P/out/hd.log; exit 1; }
    mmd $M ::fgdata ::libs || exit 1
    mcopy $M -s -Q -m $SRC/fgdata/* ::fgdata/ >> $P/out/hd.log 2>&1 || { tail -3 $P/out/hd.log; exit 1; }
    mcopy $M $BUILD/bin/pc-x86_64/AROS/Libs/openal.library ::libs/ || exit 1 ;;
release)
    X=$P/out/release-extracted
    rm -rf $X && mkdir -p $X && lha xqw=$X $P/out/release/*-x86_64.lha || exit 1
    (cd $X/FlightGear && sha256sum --quiet -c $P/out/release/drawer.sha256) || { echo "the archive's files differ from drawer.sha256"; exit 1; }
    [ "$(cd $X/FlightGear && find . -type f | wc -l)" = "$(wc -l < $P/out/release/drawer.sha256)" ] || { echo "file count differs"; exit 1; }
    rm -f $IMG && truncate -s ${SIZE_GB}G $IMG || exit 1
    echo "start=$START, type=c" | /usr/sbin/sfdisk -q $IMG || exit 1
    /usr/sbin/mkfs.vfat -F 32 -n FG -h $START --offset $START $IMG $(( (SIZE_GB * 1024 * 1024 * 1024 / 512 - START) / 2 )) \
        > $P/out/hd.log 2>&1 || { tail -3 $P/out/hd.log; exit 1; }
    mcopy $M -s -Q -m $X/FlightGear :: || exit 1
    for f in "${@:2}"; do mcopy $M -Q "$f" :: || exit 1; done
    mdir $M -/ -b :: | head -20
    echo "$(du -h $IMG | cut -f1) used of $(ls -l $IMG | awk '{print $5}') bytes"
    exit 0 ;;
fgfs|home)
    [ -f $IMG ] || { echo "no $IMG: make-hd.sh new"; exit 1; }
    [ "$1" = home ] && { mdeltree $M ::.fgfs 2>/dev/null; echo "home emptied"; } ;;
*)  echo "usage: $0 new|fgfs|home|release [FILE...]"; exit 2 ;;
esac
$BUILD/bin/linux-x86_64/tools/x86_64-pc-aros-strip --strip-unneeded -o $P/out/fgfs.stripped \
    $P/work/build-flightgear/src/Main/fgfs || exit 1
mcopy $M -o $P/out/fgfs.stripped ::fgfs || exit 1
mdir $M :: | tail -8
echo "$(du -h $IMG | cut -f1) used of $(ls -l $IMG | awk '{print $5}') bytes"
