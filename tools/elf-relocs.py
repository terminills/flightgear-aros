#!/usr/bin/env python3
# elf-relocs.py FILE...: count the relocation types of x86_64 ELF objects (and members of ar archives) and name
# the symbols of the GOT ones, which AROS's loader does not take. Exit status 1 if any file has one.
import struct, sys, collections
NAMES = {1:'64', 2:'PC32', 4:'PLT32', 9:'GOTPCREL', 10:'32', 11:'32S', 24:'PC64', 25:'GOTOFF64', 26:'GOTPC32',
         27:'GOT64', 28:'GOTPCREL64', 29:'GOTPC64', 31:'PLTOFF64', 41:'GOTPCRELX', 42:'REX_GOTPCRELX'}
GOT = {3, 9, 22, 25, 26, 27, 28, 29, 30, 31, 41, 42}
def elf(data, label, tot, got):
    if data[:4] != b'\x7fELF' or data[4] != 2: return
    shoff, = struct.unpack_from('<Q', data, 0x28)
    shentsize, shnum = struct.unpack_from('<HH', data, 0x3A)
    if shnum == 0: shnum, = struct.unpack_from('<Q', data, shoff + 0x20)    # more than 65279 sections
    sh = [struct.unpack_from('<IIQQQQIIQQ', data, shoff + i*shentsize) for i in range(shnum)]
    for s in sh:
        if s[1] != 4: continue                      # SHT_RELA
        symtab = sh[s[6]]; strtab = sh[symtab[6]]
        for off in range(s[4], s[4] + s[5], 24):
            info, = struct.unpack_from('<Q', data, off + 8)
            t = info & 0xffffffff
            tot[t] += 1
            if t in GOT:
                st_name, = struct.unpack_from('<I', data, symtab[4] + (info >> 32)*24)
                e = data.index(b'\0', strtab[4] + st_name)
                got[(label, data[strtab[4] + st_name:e].decode(errors='replace'))] += 1
rc = 0
for f in sys.argv[1:]:
    data = open(f, 'rb').read()
    tot = collections.Counter(); got = collections.Counter()
    if data[:8] == b'!<arch>\n':
        p = 8; longnames = b''
        while p + 60 <= len(data):
            name = data[p:p+16].decode().rstrip(); size = int(data[p+48:p+58]); body = data[p+60:p+60+size]
            if name == '//': longnames = body
            elif name.startswith('/') and name[1:].isdigit():
                o = int(name[1:]); name = longnames[o:longnames.index(b'/\n', o)].decode()
            elf(body, name.rstrip('/'), tot, got)
            p += 60 + size + (size & 1)
    else:
        elf(data, '', tot, got)
    g = sum(got.values())
    if g: rc = 1
    print('%s: %d relocations, %d GOT  %s' % (f.split('/')[-1], sum(tot.values()), g,
          ' '.join('%s=%d' % (NAMES.get(t, t), n) for t, n in tot.most_common() if t in GOT)))
    members = collections.Counter()
    for (m, s), n in got.items(): members[m] += n
    for m, n in members.most_common(8):
        syms = [s for (mm, s), k in got.most_common() if mm == m][:6]
        print('    %-40s %6d  %s' % (m, n, ' '.join(syms)[:110]))
sys.exit(rc)
