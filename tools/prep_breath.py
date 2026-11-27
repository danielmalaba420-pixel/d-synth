#!/usr/bin/env python3
"""The Prayer = Daniel Malaba's 'The Breath' pad multisamples (22 notes, 3 velocity layers: 063 / 095 / 127).
Every file was checked: its pitch matches the MIDI note in its name (within 0.15 semitone) and it holds one note
only. Each note keeps the loudness ratio between its layers (normalised on the loudest layer); a long loop is
built in the sustain (from 40 % of the take to its end, 2.5 s equal-power crossfade). Instrument 22.
Usage: prep_breath.py SRC_DIR RAW_BANK FLAC_BANK"""
import glob, os, re, sys
import numpy as np, soundfile as sf
SRC, RAW, FLAC = sys.argv[1:4]
SR = 44100
LAYERS = [(63, 1, 79), (95, 80, 110), (127, 111, 127)]
notes = sorted({int(re.search(r'_(\d{3})_\d{3}\.wav$', f).group(1)) for f in glob.glob(os.path.join(SRC, 'The_Breath_*_*.wav'))})
lines = []
for n in notes:
    takes = []
    for v, lo, hi in LAYERS:
        x, sr = sf.read(os.path.join(SRC, f'The_Breath_{n:03d}_{v:03d}.wav'), always_2d=True); assert sr == SR
        a = np.abs(x).max(1); st = int(np.argmax(a > a.max() * 10 ** (-50 / 20))); x = x[max(0, st - 64):]
        L = len(x); le = L - int(0.05 * SR); ls = int(max(0.4 * L, 3.0 * SR)); xf = int(min(2.5 * SR, 0.5 * (le - ls)))
        z = x[:le].copy(); g = np.linspace(0, 1, xf)[:, None]
        z[le - xf:le] = x[le - xf:le] * np.cos(g * np.pi / 2) + x[ls - xf:ls] * np.sin(g * np.pi / 2)
        lev = np.sqrt(np.mean(z[int(0.5 * SR):].mean(1) ** 2))
        takes.append((v, lo, hi, z, ls, le, lev))
    top = max(t[6] for t in takes)
    for v, lo, hi, z, ls, le, lev in takes:
        scale = 0.98 / np.abs(z).max()
        pcm = np.clip(np.round(z * scale * 32767), -32768, 32767).astype('<i2')
        name = f'i22_br_{n:03d}_v{v}'
        pcm.tofile(os.path.join(RAW, name + '.raw'))
        sf.write(os.path.join(FLAC, name + '.flac'), pcm.reshape(-1, 2), SR, subtype='PCM_16')
        gain = (0.12 / top) / scale                       # same gain for every layer of a note: dynamics from the takes
        for d, ext in ((RAW, 'raw'), (FLAC, 'flac')):
            lines.append((d, f'22 {n} {lo} {hi} {len(z)} 2 {SR:.2f} {ls} {le} {gain:.6f} {name}.{ext}'))
    print('note', n, 'ok')
for d in (RAW, FLAC):
    for f in os.listdir(d):
        if f.startswith('i22_') and not f.startswith('i22_br_'): os.remove(os.path.join(d, f))
    p = os.path.join(d, 'samples.txt')
    old = [l for l in open(p) if not l.startswith('22 ')]
    open(p, 'w').write(''.join(old).rstrip('\n') + '\n' + '\n'.join(l for dd, l in lines if dd == d) + '\n')
print(len(lines) // 2, 'zones')
