#!/bin/bash
# Build fgrcc, FlightGear's resource compiler, for the build host. FlightGear's build runs it to write
# FlightGear-resources.cxx, so a cross build cannot use the one it would build for AROS. A host SimGear would
# need Boost, curl and OpenSceneGraph development files on the host; fgrcc needs only the few SimGear sources
# listed here, so they are compiled directly. Boost headers: the AROS SDK's (header-only use).
# Result: work/host/bin/fgrcc (build-flightgear.sh passes it as FGRCC_EXECUTABLE).
. "$(dirname "$0")/env.sh"
H=$P/work/host
SG=$SRC/simgear
mkdir -p $H/inc $H/gen/simgear $H/obj $H/bin $P/logs
ln -sfn $SDK/include/boost $H/inc/boost
cat > $H/gen/simgear/simgear_config.h <<'EOC'
#define HAVE_SYS_TIME_H
#define HAVE_UNISTD_H 1
#define HAVE_GETTIMEOFDAY
#define HAVE_RINT
#define HAVE_TIMEGM
#define HAVE_MKDTEMP
#define HAVE_WORKING_STD_REGEX
#define HAVE_STD_INDEX_SEQUENCE
#define HAVE_STD_REMOVE_CV_T
#define HAVE_STD_ENABLE_IF_T
#define HAVE_STD_BOOL_CONSTANT
#define GCC_ATOMIC_BUILTINS_FOUND
EOC
cp $P/work/build-simgear/simgear/version.h $H/gen/simgear/version.h
cp $H/gen/simgear/simgear_config.h $H/gen/simgear_config.h
cat > $H/gen/simgear_expat_config.h <<'EOC'
#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H 1
#define BYTEORDER 1234
#define HAVE_GETRANDOM
#define HAVE_FCNTL_H
#define HAVE_INTTYPES_H
#define HAVE_STDINT_H
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_SYS_STAT_H
#define HAVE_SYS_TYPES_H
#define HAVE_UNISTD_H
#define STDC_HEADERS
#define XML_CONTEXT_BYTES 1024
#define XML_DTD 1
#define XML_GE 1
#define XML_NS 1
#endif
EOC
INC="-I$H/gen -I$H/gen/simgear -I$SG -I$SG/3rdparty/expat -I$H/inc"
DEF="-DHAVE_CONFIG_H -DHAVE_SIMGEAR_EXPAT_CONFIG_H -DXML_STATIC"
CXXSRC="
simgear/misc/sg_path.cxx simgear/misc/strutils.cxx simgear/misc/argparse.cxx
simgear/xml/easyxml.cxx
simgear/io/iostreams/sgstream.cxx simgear/io/iostreams/zlibstream.cxx simgear/io/iostreams/CharArrayStream.cxx
simgear/io/iostreams/gzfstream.cxx
simgear/structure/exception.cxx
simgear/embedded_resources/EmbeddedResource.cxx
simgear/debug/logstream.cxx simgear/debug/LogCallback.cxx simgear/debug/LogEntry.cxx
simgear/props/props.cxx simgear/props/PropertyInterpolationMgr.cxx simgear/props/PropertyInterpolator.cxx simgear/props/easing_functions.cxx simgear/structure/SGSourceLocation.cxx
simgear/debug/logdelta.cxx simgear/threads/SGThread.cxx simgear/timing/timestamp.cxx simgear/misc/sg_dir.cxx
simgear/debug/BufferedLogCallback.cxx simgear/debug/ErrorReportingCallback.cxx simgear/debug/Reporting.cxx
$HOST_EXTRA"
CSRC="3rdparty/expat/hashtable.c 3rdparty/expat/xmlparse.c 3rdparty/expat/xmlrole.c 3rdparty/expat/xmltok.c
3rdparty/expat/random_getrandom.c simgear/package/md5.c"
L=$P/logs/host-tools.log; : > $L; O=
for s in $CXXSRC; do
    o=$H/obj/$(echo $s | tr / _).o; O="$O $o"
    g++ -std=c++17 -O1 -w -ffunction-sections -fdata-sections $DEF $INC -c $SG/$s -o $o >> $L 2>&1 || echo "failed: $s"
done
for s in $CSRC; do
    o=$H/obj/$(echo $s | tr / _).o; O="$O $o"
    gcc -O1 -w -ffunction-sections -fdata-sections $DEF $INC -c $SG/$s -o $o >> $L 2>&1 || echo "failed: $s"
done
g++ -std=c++17 -O1 -w $DEF $INC -I$SRC/flightgear/src/EmbeddedResources \
    $SRC/flightgear/src/EmbeddedResources/fgrcc.cxx $O -Wl,--gc-sections -lz -lpthread -o $H/bin/fgrcc >> $L 2>&1
echo "fgrcc rc $?"
