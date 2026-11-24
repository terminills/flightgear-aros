#!/bin/bash
# Rebuild everything fgfs is made of, in order, from the sources in upstream/src: the compat library,
# OpenSceneGraph, PLIB, liblzma, SimGear, fgrcc for the build machine, FlightGear (target fgfs). Stops at the first step that fails.
# Usage: rebuild-all.sh   (logs in logs/, a summary line per step on stdout)
P=$(cd "$(dirname "$0")" && pwd)
step() {  # script args...
    out=$($P/"$@" 2>&1 | tail -1)
    echo "$(date +%H:%M:%S) $*: $out"
    case "$out" in *"rc 0"|libfgaros*|warnings*) ;; *) echo "STOPPED at $*"; exit 1 ;; esac
}
step build-compat.sh
for c in osg plib xz simgear; do
    step build-$c.sh configure
    step build-$c.sh make
    step build-$c.sh install
done
# fgrcc for the build machine: needs the version.h SimGear's configure wrote
step build-host-tools.sh
step build-flightgear.sh configure
step build-flightgear.sh make fgfs
ls -la $P/work/build-flightgear/src/Main/fgfs
