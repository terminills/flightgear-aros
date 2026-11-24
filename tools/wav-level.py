#!/usr/bin/env python3
# wav-level.py FILE [SECONDS-PER-ROW]: what the test VM played (QEMU writes the guest's sound output to a WAV):
# length, peak, the share of clipped samples, and the level over time, so that "sound works" is read from the
# file. QEMU only writes while the guest's sound device runs, so the file's length is the time sound was open.
import sys, wave, array, math

step = float(sys.argv[2]) if len(sys.argv) > 2 else 5.0
try:
    w = wave.open(sys.argv[1], 'rb')
    ch, rate, n = w.getnchannels(), w.getframerate(), w.getnframes()
    assert w.getsampwidth() == 2, "16-bit samples expected"
    read = w.readframes
except wave.Error:
    # QEMU writes the header when it closes the file; of a running VM: 44 bytes to skip, then 44100 Hz stereo
    import os
    f = open(sys.argv[1], 'rb'); f.seek(44)
    ch, rate, n = 2, 44100, (os.path.getsize(sys.argv[1]) - 44) // 4
    read = lambda frames: f.read(frames * 4)
print(f"{sys.argv[1]}: {n / rate:.1f} s, {rate} Hz, {ch} channels")
peak = clipped = total = quiet = rows = 0
t = 0.0
while True:
    a = array.array('h', read(int(rate * step)))
    if not a: break
    rms = math.sqrt(sum(x * x for x in a) / len(a))
    p = max(abs(x) for x in a)
    peak = max(peak, p); clipped += sum(1 for x in a if abs(x) >= 32767); total += len(a)
    rows += 1; quiet += rms < 8
    db = 20 * math.log10(rms / 32768) if rms >= 1 else -99.0
    print(f"  {t:6.0f} s  rms {db:6.1f} dB  peak {p:5d}  {'#' * max(0, int((db + 72) / 3))}")
    t += len(a) / ch / rate
if total:
    print(f"peak {peak}, clipped {100 * clipped / total:.2f}%, silent rows {quiet} of {rows}")
else:
    print("no samples: nothing opened the sound device")
