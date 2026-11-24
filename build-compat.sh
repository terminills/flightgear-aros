#!/bin/bash
# Build aros/compat (what the port needs and AROS's C library does not have, and the POSIX socket calls as
# real functions) into work/prefix: lib/libfgaros.a, include/fgaros.h (force-included by the builds) and
# include/fgaros/ (headers that stand in front of the SDK's: the builds add it with -isystem).
. "$(dirname "$0")/env.sh"
T=$BUILD/bin/linux-x86_64/tools/crosstools
W=$P/work/build-compat
mkdir -p $W $PREFIX/lib $PREFIX/include $P/logs
rm -f $W/*.o $PREFIX/lib/libfgaros.a
: > $P/logs/compat.log
for f in $P/aros/compat/*.c; do
    $T/x86_64-aros-gcc -O2 -Wall $PATHMAP_FLAGS -isystem $P/aros/compat/include -DBSDSOCKET_NOLIBDEFINES \
        -c -o $W/$(basename $f .c).o $f >> $P/logs/compat.log 2>&1 ||
        { echo "FAILED: $f"; tail -20 $P/logs/compat.log; exit 1; }
done
$T/x86_64-aros-ar rcs $PREFIX/lib/libfgaros.a $W/*.o || exit 1
cp -p $P/aros/compat/fgaros.h $PREFIX/include/ || exit 1
rm -rf $PREFIX/include/fgaros && cp -rp $P/aros/compat/include $PREFIX/include/fgaros || exit 1
echo "libfgaros.a: $($T/x86_64-aros-ar t $PREFIX/lib/libfgaros.a | tr '\n' ' ')"
echo "warnings: $(/usr/bin/grep -c 'warning:' $P/logs/compat.log)"
