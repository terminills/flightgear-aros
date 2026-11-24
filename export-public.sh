#!/bin/bash
# export-public.sh DIR: the files of this repository that are published, as committed (HEAD), into DIR: the
# paths listed in aros/release/public-files, aros/release/README.md as README.md and aros/release/LICENSE as
# LICENSE. Files already in DIR that are no longer exported are removed (DIR/.git is left alone). The source
# archive of make-release.sh carries the same tree. Fails if a file names a directory of the build machine.
P=$(cd "$(dirname "$0")" && pwd)
D=${1:?usage: export-public.sh DIR}
mkdir -p $D && D=$(cd $D && pwd) || exit 1
find $D -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} + || exit 1
git -C $P archive HEAD $(cat $P/aros/release/public-files) | tar -x -C $D || exit 1
mv $D/aros/release/README.md $D/README.md && mv $D/aros/release/LICENSE $D/LICENSE || exit 1
echo "evidence/" >> $D/.gitignore
n=$(command grep -rIl '/hom[e]/\|/ru[n]/media' $D --exclude-dir=.git | tee /dev/stderr | wc -l)
[ "$n" = 0 ] || { echo "$n files name a build machine directory"; exit 1; }
echo "$(find $D -type f -not -path '*/.git/*' | wc -l) files in $D"
