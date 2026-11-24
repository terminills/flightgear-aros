#!/bin/bash
# Fetch the upstream sources into upstream/src at the commits this port is made against, and put the AROS
# patches of aros/patches/<name> on a branch "aros". A source directory that already exists is left alone.
P=$(cd "$(dirname "$0")" && pwd)
get() {  # name url commit
    d=$P/upstream/src/$1
    [ -d $d ] && { echo "$1: present ($(git -C $d log -1 --format=%h))"; return; }
    mkdir -p $d && cd $d && git init -q && git remote add origin $2 &&
        git fetch -q --depth 1 origin $3 && git checkout -q -b aros FETCH_HEAD || { echo "$1: fetch failed"; exit 1; }
    ls $P/aros/patches/$1/*.patch >/dev/null 2>&1 && { git am -q $P/aros/patches/$1/*.patch || exit 1; }
    echo "$1: $(git log -1 --format=%h)"
}
get openscenegraph https://gitlab.com/flightgear/openscenegraph.git 1524894a6d30fe8921c53a1e04bf08b3e5205cad
get simgear        https://gitlab.com/flightgear/simgear.git        c903a7b790473f14e196fbec91d5ecd828f7d386
get flightgear     https://gitlab.com/flightgear/flightgear.git     7241807173248abd838e2ab11337fbc5b2ae20b0
get plib           https://git.code.sf.net/p/libplib/code           aab82eaf3aca7b82ad1e9a47fcc5d1abcf662fa0
get xz             https://github.com/tukaani-project/xz.git        d3e650e63c110e830fd5391e7f8b45df0b91d3da
get fgdata         https://gitlab.com/flightgear/fgdata.git         74b24bfc89c4b5c98f7414d71a4f5fcac7764b9e
