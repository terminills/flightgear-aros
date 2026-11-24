#!/usr/bin/env python3
# vm-stack.py MONITOR-SOCKET WORDS OBJECT... : who called the code a running AROS guest is in. Stops the VM, reads
# WORDS 8-byte words upwards from the stack pointer through the QEMU monitor, keeps those that follow a call
# instruction, finds the code around each in the given AROS objects (as vm-profile.py does) and prints the chain,
# innermost first; then lets the VM run again. Without frame pointers this is a scan, not an unwinding: a stale
# return address left in the stack by an earlier call shows up too. "?" = code of none of the objects (or a run
# with relocated instructions in it). The VM is stopped at a moment the CPU runs user-mode code.
import sys, re, struct, importlib.util, os
spec = importlib.util.spec_from_loader('vmprofile', loader=None)
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'vm-profile.py')).read()
vp = importlib.util.module_from_spec(spec); exec(src[:src.index('\ns = mon_open(')], vp.__dict__)

s = vp.mon_open(sys.argv[1]); words = int(sys.argv[2]); objs = [o for o in map(vp.Obj, sys.argv[3:]) if o.ok]
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
        if k >= 2 and base <= addr < base + o.sh[sec][5]: return '%s: %s' % (o.name, o.symbol(sec, addr - base))
    return None
import time
for attempt in range(200):
    # a stop inside an interrupt or the scheduler says nothing about the program: wait for user-mode code
    vp.mon(s, 'stop'); r = vp.mon(s, 'info registers')
    if 'CPL=3' in r and 'HLT=0' in r: break
    vp.mon(s, 'cont'); time.sleep(0.02)
try:
    rip = int(re.search(r'RIP=([0-9a-f]+)', r).group(1), 16); rsp = int(re.search(r'RSP=([0-9a-f]+)', r).group(1), 16)
    print('RIP %x  %s' % (rip, where(rip) or '?'))
    x = vp.mon(s, 'x/%dgx 0x%x' % (words, rsp))
    stack = [int(v, 16) for l in x.splitlines() if ':' in l for v in re.findall(r'0x([0-9a-f]{16})', l.split(':', 1)[1])]
    for i, v in enumerate(stack):
        if not 0x100000 <= v < 1 << 40: continue
        b = mem(v - 8, 8)
        if len(b) != 8: continue
        # call rel32; call *reg; call *disp8(reg); call *disp32(reg); the same with a SIB or REX byte in front
        if not (b[3] == 0xe8 or (b[6] == 0xff and b[7] & 0xf8 == 0xd0) or (b[5] == 0xff and b[6] & 0xf8 == 0x50)
                or (b[2] == 0xff and b[3] & 0xf8 == 0x90) or (b[4] == 0xff and b[5] & 0xc7 == 0x54)
                or (b[1] == 0xff and b[2] & 0xc7 == 0x94)): continue
        print('  rsp+%-5x %x  %s' % (i * 8, v, where(v) or '?'))
finally:
    vp.mon(s, 'cont')
