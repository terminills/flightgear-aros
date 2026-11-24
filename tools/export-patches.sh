#!/bin/bash
# export-patches.sh: writes aros/patches/<name> again from the branch "aros" of upstream/src/<name>, one patch
# per commit on top of the commit fetch-upstream.sh names. Trailer lines of the local commits (anything of the
# form "Word-Word: value" after the message) are left out, and so is git's version signature.
P=$(cd "$(dirname "$0")/.." && pwd)
while read -r get name url base; do
    [ "$get" = get ] || continue
    d=$P/upstream/src/$name
    [ "$(git -C $d rev-list --count $base..aros)" = 0 ] && continue
    rm -rf $P/aros/patches/$name && mkdir -p $P/aros/patches/$name
    git -C $d format-patch -q --zero-commit --no-signature -o $P/aros/patches/$name $base..aros || exit 1
    for f in $P/aros/patches/$name/*.patch; do
        python3 - "$f" <<'PY' || exit 1
import re, sys
p = sys.argv[1]
head, sep, rest = open(p).read().partition("\n---\n")
lines = head.split("\n")
while lines and re.match(r"[A-Z][A-Za-z]*(-[A-Za-z]+)+: ", lines[-1]):
    lines.pop()
while lines and lines[-1] == "":
    lines.pop()
open(p, "w").write("\n".join(lines) + "\n" + sep[1:] + rest)
PY
    done
    echo "$name: $(ls $P/aros/patches/$name | wc -l) patches"
done < $P/fetch-upstream.sh
