#!/usr/bin/env python3
# vm-profile.py MONITOR-SOCKET SAMPLES OBJECT... : where a running AROS guest spends its time. Takes SAMPLES
# readings of the CPU's instruction pointer through the QEMU monitor (with the 16 bytes of code there), finds
# those bytes in the given AROS objects (the loader places them anywhere, so the code itself is the way back)
# and prints a count per object and function. Good for objects with one code section (libraries, handlers);
# an object with a section per function (fgfs) is only counted where a sample happens to place one, and a
# stripped object gives the nearest exported name, not the function.
import socket, struct, sys, re, time, collections, bisect

def mon_open(path):
    s = socket.socket(socket.AF_UNIX); s.connect(path); s.settimeout(5)
    buf = b''
    while not buf.endswith(b'(qemu) '): buf += s.recv(65536)
    return s
def mon(s, cmd):
    s.sendall(cmd.encode() + b'\n'); buf = b''
    while not buf.endswith(b'(qemu) '): buf += s.recv(65536)
    return re.sub(rb'\x1b\[[0-9;]*[A-Za-z]', b'', buf).decode(errors='replace')

class Obj:
    def __init__(self, path):
        self.path = path; self.name = path.split('/')[-1]; d = self.data = open(path, 'rb').read()
        self.ok = d[:4] == b'\x7fELF' and d[4] == 2
        if not self.ok: return
        shoff, = struct.unpack_from('<Q', d, 0x28)
        shentsize, shnum = struct.unpack_from('<HH', d, 0x3A)
        if shnum == 0: shnum, = struct.unpack_from('<Q', d, shoff + 0x20)
        self.sh = sh = [struct.unpack_from('<IIQQQQIIQQ', d, shoff + i*shentsize) for i in range(shnum)]
        self.code = sorted((s[4], s[4] + s[5], i) for i, s in enumerate(sh) if s[1] == 1 and s[2] & 4)
        self.syms = collections.defaultdict(list)
        xindex = next((s for s in sh if s[1] == 18), None)
        for s in sh:
            if s[1] != 2: continue
            st = sh[s[6]]
            for n in range(s[5] // 24):
                name, info, other, shndx, value, size = struct.unpack_from('<IBBHQQ', d, s[4] + n*24)
                if info & 15 != 2: continue                              # STT_FUNC
                if shndx == 0xffff and xindex: shndx, = struct.unpack_from('<I', d, xindex[4] + n*4)
                e = d.index(b'\0', st[4] + name)
                self.syms[shndx].append((value, d[st[4] + name:e].decode(errors='replace')))
        for v in self.syms.values(): v.sort()
    def place(self, code, at):
        # a 16-byte run that the object has once: (section, the address the section was loaded at)
        p = self.data.find(code)
        if p < 0 or self.data.find(code, p + 1) >= 0: return None
        i = bisect.bisect_right(self.code, (p, 1 << 62, 0)) - 1
        if i < 0 or not (self.code[i][0] <= p < self.code[i][1]): return None
        return self.code[i][2], at - (p - self.code[i][0])
    def symbol(self, sec, off):
        l = self.syms.get(sec)
        j = bisect.bisect_right(l, (off, '\uffff')) - 1 if l else -1
        return l[j][1] if j >= 0 else '%s[%d]+0x%x' % (self.name, sec, off)

s = mon_open(sys.argv[1]); n = int(sys.argv[2]); samples = []
for i in range(n):
    r = mon(s, 'info registers'); m = re.search(r'RIP=([0-9a-f]+).*?CPL=(\d)', r, re.S)
    if not m: continue
    x = mon(s, 'x/48xb 0x%x' % (int(m.group(1), 16) - 32))
    code = bytes(int(b, 16) for b in re.findall(r'0x([0-9a-f]{2})\b', x))
    samples.append((int(m.group(1), 16), int(m.group(2)), 'HLT=1' in r, code))
    time.sleep(0.03)
objs = [o for o in map(Obj, sys.argv[3:]) if o.ok]
# code that is found once in an object says where that section was loaded; a relocated instruction in the run
# spoils a match, so three runs around each sample are tried and the placements counted
placed = collections.Counter()
for rip, cpl, hlt, code in samples:
    if hlt or len(code) != 48: continue
    for o in objs:
        for w in (0, 16, 32):
            r = o.place(code[w:w+16], rip - 32 + w)
            if r and r[1] % max(o.sh[r[0]][8], 1) == 0: placed[(o, r[0], r[1])] += 1
# one run alone, or a load address the section's alignment does not allow, is a chance match
placed = collections.Counter({k: n for k, n in placed.items() if n >= 2})
count = collections.Counter(); where = collections.Counter()
for rip, cpl, hlt, code in samples:
    if hlt: count['(idle: hlt)'] += 1; where['idle'] += 1; continue
    for (o, sec, base), k in placed.most_common():
        if base <= rip < base + o.sh[sec][5]:
            count[o.symbol(sec, rip - base)] += 1; where[o.name] += 1; break
    else:
        count['(not found) %x cpl%d' % (rip & ~0xfff, cpl)] += 1; where['?'] += 1
print('%d samples' % len(samples), dict(where))
for (o, sec, base), k in placed.most_common(12): print('    %s section %d at 0x%x (%d runs)' % (o.name, sec, base, k))
for f, c in count.most_common(45): print('%5d  %s' % (c, f[:150]))
