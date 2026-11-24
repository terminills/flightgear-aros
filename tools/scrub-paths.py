#!/usr/bin/env python3
"""scrub-paths.py --map DIR=NAME [--map DIR=NAME ...] FILE...: takes the build machine's directories out of
linked AROS binaries.

The port's own objects are compiled with -ffile-prefix-map (env.sh), but the static libraries of the AROS build
tree (libstdc++, OpenSSL, curl) were compiled before and carry that tree's path in __FILE__ strings and in
OpenSSL's "compiler:" line. Each occurrence of DIR is overwritten in place with NAME, filled up with "_" to
DIR's length, so nothing in the file moves. Prints the number of replacements per file and fails if a string
that names a home or removable-media directory is left.
"""
import re
import sys

maps = []
files = []
args = sys.argv[1:]
while args:
    a = args.pop(0)
    if a == "--map":
        d, n = args.pop(0).split("=", 1)
        maps.append((d.rstrip("/").encode(), n.encode()))
    else:
        files.append(a)
# the longest directory first: one may lie inside another
maps.sort(key=lambda m: -len(m[0]))

rc = 0
for name in files:
    data = open(name, "rb").read()
    total = 0
    for prefix, short in maps:
        fill = short + b"_" * (len(prefix) - len(short))
        assert len(fill) == len(prefix)
        total += data.count(prefix)
        data = data.replace(prefix, fill)
    left = re.findall(rb"[\x20-\x7e]*(?:/hom[e]/|/ru[n]/media/)[\x20-\x7e]*", data)
    open(name, "wb").write(data)
    print(f"{name}: {total} replaced, {len(left)} left")
    for text in left[:5]:
        print("  left:", text.decode())
    if left:
        rc = 1
sys.exit(rc)
