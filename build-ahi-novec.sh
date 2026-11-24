#!/bin/bash
# build-ahi-novec.sh: a second ahi.device for the tests, compiled like the build tree's but with -fno-tree-vectorize
# (into out/ahi-novec/ahi.device). The AROS build tree is read, not changed. An experiment, not a proposal: see
# README, "Sound".
. "$(dirname "$0")/env.sh"
G=$BUILD/bin/pc-x86_64/gen/workbench/devs/AHI
W=$P/work/ahi-novec
rm -rf $W; mkdir -p $W $P/out/ahi-novec $P/logs
cp -a $G/Device $W/Device && cp -a $G/config.h $G/config.status $G/Makefile $W/ || exit 1
cd $W/Device || exit 1
rm -f *.o *.a ahi.device
CF="$(sed -n 's/^CFLAGS[\t ]*= //p' Makefile) -fno-tree-vectorize"
export PATH=$BUILD/bin/linux-x86_64/tools/crosstools:$BUILD/bin/linux-x86_64/tools:$PATH
make CFLAGS="$CF" ahi.device > $P/logs/ahi-novec.log 2>&1; rc=$?
[ $rc = 0 ] && cp ahi.device $P/out/ahi-novec/ahi.device
echo "ahi-novec rc $rc"
# what is left of the registers an interrupt does not save (xmm4-15), by function
x86_64-aros-objdump -d ahi.device 2>/dev/null | awk '/^[0-9a-f]+ <.*>:$/ {fn=$2} /%[xy]mm([4-9]|1[0-5])/ {c[fn]++} END {for (f in c) print c[f], f}' | sort -rn
