#!/usr/bin/env python3
# vm-addr.py MONITOR-SOCKET OBJECT... -- ADDRESS... : which function a code address of the AROS guest is in (the
# addresses of an alert or of a "[Kernel:TLSF]" backtrace in the serial log). Reads the code around each address
# through the QEMU monitor and finds it in the given AROS objects, as vm-stack.py does. "?" = none of the objects.
import sys, re, importlib.util, os
spec = importlib.util.spec_from_loader('vmprofile', loader=None)
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'vm-profile.py')).read()
vp = importlib.util.module_from_spec(spec); exec(src[:src.index('\ns = mon_open(')], vp.__dict__)
a = sys.argv[2:]; i = a.index('--')
s = vp.mon_open(sys.argv[1]); objs = [o for o in map(vp.Obj, a[:i]) if o.ok]
def mem(addr, n):
    x = vp.mon(s, 'x/%dxb 0x%x' % (n, addr))
    return bytes(int(b, 16) for b in re.findall(r'0x([0-9a-f]{2})\b', x))
def where(addr):
    code = mem(addr - 64, 128)
    if len(code) != 128: return None
    best = {}
    for o in objs:
        for w in range(0, 112, 8):
            r = o.place(code[w:w+16], addr - 64 + w)
            if r and r[1] % max(o.sh[r[0]][8], 1) == 0: best[(o, r[0], r[1])] = best.get((o, r[0], r[1]), 0) + 1
    for (o, sec, base), k in sorted(best.items(), key=lambda x: -x[1]):
        if k >= 2 and base <= addr < base + o.sh[sec][5]:
            return '%s: %s+0x%x' % (o.name, o.symbol(sec, addr - base), addr - base)
    return None
for v in a[i+1:]:
    v = int(v, 16); print('%x  %s' % (v, where(v) or '?'))
