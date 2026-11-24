# CMake toolchain file of this port: the AROS build tree's own (work/aros-build, a link env.sh sets to its
# BUILD), with work/prefix as a
# second search root. The AROS file sets CMAKE_FIND_ROOT_PATH as a normal variable, so a -D on the command
# line does not add to it.
include("${CMAKE_CURRENT_LIST_DIR}/../../work/aros-build/bin/pc-x86_64/gen/config/conf.cmake")
get_filename_component(_fg_prefix "${CMAKE_CURRENT_LIST_DIR}/../../work/prefix" ABSOLUTE)
list(APPEND CMAKE_FIND_ROOT_PATH "${_fg_prefix}")
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
list(APPEND CMAKE_PREFIX_PATH "/")
