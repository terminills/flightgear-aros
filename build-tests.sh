#!/bin/bash
# Build the test programs in tests/ against the OpenSceneGraph in work/prefix (static) into out/tests.
. "$(dirname "$0")/env.sh"
CXX=$BUILD/bin/linux-x86_64/tools/crosstools/x86_64-aros-g++
mkdir -p $P/out/tests $P/logs
for t in ${@:-osgwin}; do
    # tests/cxxnop.cpp three ways, and tests/twice.c with -nix: what a program must be made of to leave its C
    # library state to the next one in the Shell
    SGLIBS="-L$PREFIX/lib -lSimGearCore -lcurl -lFGExpat $PREFIX/lib/liblzma.a -lFGUdns -lssl -lcrypto -lzstd -lbrotlidec -lbrotlicommon $BUILD/bin/pc-x86_64/AROS/Developer/lib/libz.static.a -latomic -lfgaros -lpthread"
    case $t in
    cxxnop) $CXX -O2 -std=c++17 -DNAME='"cxxnop"' -o $P/out/tests/$t $P/tests/cxxnop.cpp > $P/logs/test-$t.log 2>&1; echo "$t rc $?"; continue ;;
    cxxnix) $CXX -O2 -std=c++17 -DNAME='"cxxnix"' -nix -o $P/out/tests/$t $P/tests/cxxnop.cpp > $P/logs/test-$t.log 2>&1; echo "$t rc $?"; continue ;;
    cxxsg)  $CXX -O2 -std=c++17 -DNAME='"cxxsg"' -DWITH_SG $FGAROS_FLAGS -I$PREFIX/include -nix -o $P/out/tests/$t $P/tests/cxxnop.cpp $SGLIBS > $P/logs/test-$t.log 2>&1; echo "$t rc $?"; continue ;;
    # tests/xzcheck.c with the port's liblzma (build-xz.sh), and with the lzma.library AROS comes with
    xzcheck) $BUILD/bin/linux-x86_64/tools/crosstools/x86_64-aros-gcc -O2 -I$PREFIX/include -o $P/out/tests/$t $P/tests/xzcheck.c $PREFIX/lib/liblzma.a > $P/logs/test-$t.log 2>&1; echo "$t rc $?"; continue ;;
    xzchecksys) $BUILD/bin/linux-x86_64/tools/crosstools/x86_64-aros-gcc -O2 -o $P/out/tests/$t $P/tests/xzcheck.c $SDK/lib/liblzma.a > $P/logs/test-$t.log 2>&1; echo "$t rc $?"; continue ;;
    # tests/tlsdtor.cpp: whether a thread_local object is still whole when its destructor runs
    tlsdtor) $CXX -O2 -std=c++17 -o $P/out/tests/$t $P/tests/tlsdtor.cpp -lpthread > $P/logs/test-$t.log 2>&1; echo "$t rc $?"; continue ;;
    twicenix) $BUILD/bin/linux-x86_64/tools/crosstools/x86_64-aros-gcc -O2 -nix -o $P/out/tests/$t $P/tests/twice.c -lpthread > $P/logs/test-$t.log 2>&1; echo "$t rc $?"; continue ;;
    esac
    # a plain C test needs nothing of OpenSceneGraph
    if [ -f $P/tests/$t.c ]; then
        $BUILD/bin/linux-x86_64/tools/crosstools/x86_64-aros-gcc -O2 -o $P/out/tests/$t $P/tests/$t.c -lpthread > $P/logs/test-$t.log 2>&1
        echo "$t rc $?"; continue
    fi
    # a test of SimGear's network code links what fgfs links for it (curl, OpenSSL, udns, libfgaros)
    if [ $t = sgnet ]; then
        $CXX -O2 -std=c++17 $FGAROS_FLAGS -I$PREFIX/include -nix -o $P/out/tests/$t $P/tests/$t.cpp \
            -L$PREFIX/lib -lSimGearCore -lcurl -lFGExpat $PREFIX/lib/liblzma.a -lFGUdns -lssl -lcrypto -lzstd -lbrotlidec \
            -lbrotlicommon $BUILD/bin/pc-x86_64/AROS/Developer/lib/libz.static.a -latomic -lfgaros -lpthread \
            > $P/logs/test-$t.log 2>&1
        echo "$t rc $?"; continue
    fi
    $CXX -O2 -std=c++17 -I$PREFIX/include -o $P/out/tests/$t $P/tests/$t.cpp \
        -L$PREFIX/lib -losgViewer -losgGA -losgText -losgDB -losgUtil -losg -lOpenThreads \
        -lGL -lz -lpthread > $P/logs/test-$t.log 2>&1
    echo "$t rc $?"
done
