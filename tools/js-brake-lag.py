#!/usr/bin/env python3
"""How long the brake stays on after the brake button of the test stick is released (aros/run-fgfs-js).
Reads the 'jslog N:' lines of a serial log; one line per release that the log follows to the end."""
import re, sys

for path in sys.argv[1:]:
    rows = []
    for line in open(path, 'rb').read().decode('latin-1').splitlines():
        m = re.search(r'(\d+\.\d+) \[INFO\]:nasal +jslog \d+: .*brake ([\d.]+) button0 (\d)', line)
        if m:
            rows.append((float(m.group(1)), float(m.group(2)), int(m.group(3))))
    lags = []
    for i in range(1, len(rows)):
        if rows[i - 1][2] == 1 and rows[i][2] == 0:
            off = next((r[0] for r in rows[i:] if r[1] < 0.5), None)
            if off is not None:
                lags.append((rows[i][0], off - rows[i][0]))
    late = [l for l in lags if l[1] > 2.5]
    print("%s: %d log lines, %d releases, %d with the brake on more than 2.5 s after: %s" % (
        path, len(rows), len(lags), len(late), ", ".join("%.0f s at %.0f" % (l[1], l[0]) for l in late) or "-"))
