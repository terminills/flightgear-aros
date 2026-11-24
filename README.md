# FlightGear 2024.1 for AROS

A port of [FlightGear](https://www.flightgear.org) 2024.1.7 to AROS, pc-x86_64 (ABIv1): the patches to
FlightGear, SimGear, OpenSceneGraph and PLIB, a small support library, and the scripts that build all of it
against an AROS build tree with GCC 15.2.0.

The state is a first release candidate. Builds are on the
[releases page](https://github.com/terminills/flightgear-aros/releases); the `ReadMe` in the archive says what a
user needs, how to install it and what has been tested. In short: it starts, loads the default aircraft and
airport and draws them, on AROS's software GL drivers in a virtual machine, where it is far too slow to fly. It
has not been run on a hardware GL driver or on real hardware yet. Reports are welcome as
[issues](https://github.com/terminills/flightgear-aros/issues).

## What the port is made of

| Piece | What was done |
|---|---|
| OpenSceneGraph (FlightGear's fork, `release/2024-build`) | static libraries and plugins; a graphics window class for AROS (Intuition window, GL context through AROS's GLA interface, input from IDCMP); GL function lookup through `glAGetProcAddress`; OpenThreads over pthreads |
| SimGear 2024.1.7 | built against AROS's C library and bsdsocket.library; name servers and the certificate file taken from the system |
| FlightGear 2024.1.7 | the same, plus a joystick backend for AROS's game controller subsystem (controller.hidd) |
| PLIB 1.8.5 | a platform branch; its own joystick library stays off |
| `aros/compat` (libfgaros) | the POSIX socket calls over bsdsocket.library with one library base per thread, `select()`, `mmap()` for reading files, `timegm()`, no volume requesters |
| `aros/unpack` (FGUnpack) | downloads FlightGear's data archive and unpacks it as it arrives, with SimGear's HTTP client and archive reader |
| curl, OpenAL | built from AROS contrib in the AROS build tree (`build-deps.sh`) |
| liblzma | built from xz's source and linked statically (`build-xz.sh`) |

The changes to the upstream sources are in `aros/patches`, one series per project; each patch says in its message
what it is for.

## Building

You need an AROS source tree at commit 21200a1a87 or later with a pc-x86_64 build tree next to it, configured
with `--with-gcc-version=15.2.0` and built including contrib, and on the build machine CMake, Ninja, git and a
C++ compiler (for one host tool).

    echo "BUILD=/path/to/aros/build-tree" > local.env
    ./fetch-upstream.sh      # upstream sources at the port's commits, with aros/patches applied
    ./build-deps.sh          # curl, OpenAL and llvmpipe.hidd in the AROS build tree
    ./rebuild-all.sh         # libfgaros, OpenSceneGraph, PLIB, liblzma, SimGear, FlightGear
    ./build-unpack.sh        # FGUnpack
    ./make-release.sh rc1    # the release drawer and archives in out/release

`fetch-upstream.sh` also fetches FlightGear's data (2 GB), which the tests use.

## Layout

    env.sh              paths (AROS build tree, SDK, install prefix)
    fetch-upstream.sh   upstream sources at the port's commits + aros/patches
    build-deps.sh       curl, OpenAL, llvmpipe in the AROS build tree
    build-compat.sh     aros/compat: libfgaros.a and the headers that stand in front of the SDK's
    build-osg.sh        OpenSceneGraph: configure | make | install (into work/prefix)
    build-plib.sh       PLIB: configure | make | install
    build-xz.sh         liblzma: configure | make | install
    build-simgear.sh    SimGear: configure | make | install
    build-host-tools.sh fgrcc (FlightGear's resource compiler) for the build machine
    build-flightgear.sh FlightGear: configure | make [target] | install (`make fgfs`: "all" stops in the
                        standalone JSBSim program, which the port does not need)
    build-unpack.sh     FGUnpack
    rebuild-all.sh      all of the above in order
    make-release.sh     the release drawer, the archives and the source archive
    export-public.sh    the published files of the repository, as this repository has them
    build-tests.sh      test programs of tests/ (into out/tests)
    package-test.sh     a test CD (volume FGTEST) with the test programs and one of the scripts aros/run-*
    make-hd.sh          a FAT32 hard disk image for a virtual machine, with the release drawer on it
    build-ahi-novec.sh  an ahi.device without auto-vectorisation, for the sound tests only
    aros/patches/       the changes to the upstream sources, as patch series
    aros/compat/        libfgaros
    aros/unpack/        FGUnpack
    aros/cmake/         the CMake toolchain file
    aros/release/       what goes into the release drawer as it is: ReadMe, options file, UseLLVMpipe
    aros/local-build/   a change to the AROS build tree that is not in AROS (see llvmpipe)
    aros/run-*, *.xml   AmigaDOS scripts and FlightGear configuration files of the tests
    tests/              small programs for single questions (a GL window, sockets and HTTP, vector registers,
                        destructors of thread-local objects, the xz decoder)
    tools/              helpers for the build machine (relocation check, path scrubbing, archive writer, ...)

## llvmpipe

AROS builds `llvmpipe.hidd` for its LLVM toolchain only. For this port it is built in a GCC build tree with
`aros/local-build/master-llvmpipe-with-gcc.patch` and `OPT_LLVM=20.1.0` (`build-deps.sh`), and the release
carries the result as an optional driver. The patch is a local build change, not a proposal for AROS.

## Rules the build follows

- Nothing is compiled `-fPIC`/`-fPIE`: AROS loads relocatable objects and the x86_64 loader takes no global offset
  table relocations ("file is not executable"). `tools/elf-relocs.py work/build-flightgear/src/Main/fgfs` checks.
- `fgfs` is linked with `-nix`: the C library then takes Unix path syntax (`/PROGDIR/fgdata`, `/RAM Disk/.fgfs`),
  which is what SGPath calls an absolute path. Without `HOME`, the home directory is the program's drawer;
  `FG_HOME` and `--fg-root` work as everywhere else.
- No volume requesters: FGData names files as `/Aircraft/...`, which FlightGear tries as written before it looks
  under FG_ROOT; on AROS that is the volume `Aircraft:`. `main()` calls `fgaros_no_requesters()` (libfgaros).
- Static zlib and FreeType (the stub libraries lack `gzgetc()` and `FT_Outline_Get_Orientation()`); AROSTCP's
  `libnet.a` is not linked (its `select()` and friends clash with libfgaros).
- No C++ exception leaves a callback that an AROS shared library called (the PNG reader leaves libpng's error
  callback by `longjmp`), and objects that live per thread and have a destructor sit behind a pthread key of
  their own (`SGThreadObject`) instead of `thread_local`.
- The release binaries are stripped, and the build machine's directory names are taken out of them
  (`-ffile-prefix-map` for what the port compiles, `tools/scrub-paths.py` for the static libraries of the AROS
  build tree); `make-release.sh` refuses to pack anything that still names one.

## Licence

FlightGear, SimGear and PLIB are under the GNU General Public License or Library General Public License,
OpenSceneGraph under the OpenSceneGraph Public License; the patches follow the licence of the project they
change. The port's own files (everything in this repository outside `aros/patches`) are under the GNU General
Public License, version 2 or later; see `LICENSE`.

This port is not made or supported by the FlightGear project.
