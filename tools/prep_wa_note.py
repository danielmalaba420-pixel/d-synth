#!/usr/bin/env python3
"""The Prayer, playable note by note: Daniel Malaba's Worship Atmosphere take in E (an almost pure E, no fifth)
is transposed with Rubber Band R3 (--fine, plain shift so low notes really sound low) to a zone every 3 semitones
from C2 to C6; each zone is retuned to A440, looped (8 s -> 44 s, 3 s equal-power crossfade) and written as
instrument 22 into the raw bank and the FLAC bank. The engine then plays one zone per key, like any sampled pad.
Usage: prep_wa_note.py SRC_E_TAKE RAW_BANK FLAC_BANK"""
import os, re, subprocess, sys, tempfile, warnings
import numpy as np, soundfile as sf, librosa
warnings.filterwarnings('ignore')
SRC, RAW, FLAC = sys.argv[1:4]
SR = 44100; ROOT = 52; SRC_CENTS = 4.0          # the E take sits on E3 and is 4 cents sharp
x, sr = sf.read(SRC, always_2d=True); assert sr == SR
tmp = tempfile.mkdtemp(); src = os.path.join(tmp, 'src.wav'); sf.write(src, x[:int(46 * SR)], SR, subtype='FLOAT')
lines = []
for R in range(36, 85, 3):
    out = os.path.join(tmp, f'z{R}.wav')
    subprocess.run(['rubberband', '--fine', '-q', '-p', f'{(R - ROOT) - SRC_CENTS / 100:.3f}', src, out], check=True, capture_output=True)
    y, _ = sf.read(out, always_2d=True)
    m = y.mean(1)[SR * 10:SR * 30]
    cents = 100 * float(librosa.estimate_tuning(y=librosa.resample(m, orig_sr=SR, target_sr=22050), sr=22050))
    rate = SR * 2 ** (-cents / 1200)
    ls, le, xf = int(8 * SR), int(44 * SR), int(3 * SR)
    z = y[:le].copy(); g = np.linspace(0, 1, xf)[:, None]
    z[le - xf:le] = y[le - xf:le] * np.cos(g * np.pi / 2) + y[ls - xf:ls] * np.sin(g * np.pi / 2)
    lev = np.sqrt(np.mean(z.mean(1)[int(0.5 * SR):] ** 2)); scale = 0.98 / np.abs(z).max()
    pcm = np.clip(np.round(z * scale * 32767), -32768, 32767).astype('<i2')
    name = f'i22_wan_{R:03d}'
    pcm.tofile(os.path.join(RAW, name + '.raw'))
    sf.write(os.path.join(FLAC, name + '.flac'), pcm.reshape(-1, 2), SR, subtype='PCM_16')
    gain = (0.12 / max(lev, 1e-6)) / scale
    for d, ext in ((RAW, 'raw'), (FLAC, 'flac')):
        lines.append((d, f'22 {R} 0 127 {len(z)} 2 {rate:.2f} {ls} {le} {gain:.6f} {name}.{ext}'))
    print(f'zone {R}: {cents:+.0f} cents corrected')
for d in (RAW, FLAC):
    for f in os.listdir(d):
        if f.startswith('i22_wa_'): os.remove(os.path.join(d, f))
    p = os.path.join(d, 'samples.txt')
    old = [l for l in open(p) if not l.startswith('22 ')]
    open(p, 'w').write(''.join(old).rstrip('\n') + '\n' + '\n'.join(l for dd, l in lines if dd == d) + '\n')
print('ok')
