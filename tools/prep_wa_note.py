#!/usr/bin/env python3
"""The Prayer, warm and playable note by note: Daniel Malaba's Worship Atmosphere take in E (an almost pure E3+E4,
no fifth) becomes one zone every 3 semitones from C2 to C6 by VARISPEED only (soxr VHQ resampling, like a tape
played slower/faster): no phase-vocoder processing, so nothing can sound phasey or metallic. Each zone gets a
gentle warm lowpass (2nd-order, Q 0.6, 4.5 kHz, lower in the top zones) and a soft roll-off of the sub rumble,
is tuned to A440 (the take's +4 cents are taken out in the speed ratio), looped with an equal-power crossfade and written as instrument 22 into the raw and FLAC banks.
Usage: prep_wa_note.py SRC_E_TAKE RAW_BANK FLAC_BANK [WAV_OUT_DIR]"""
import os, sys, warnings
import numpy as np, soundfile as sf, soxr
from scipy.signal import butter, sosfiltfilt
warnings.filterwarnings('ignore')
SRC, RAW, FLAC = sys.argv[1:4]; WAV = sys.argv[4] if len(sys.argv) > 4 else None
SR = 44100; ROOT = 52; SRC_CENTS = 4.0
x, sr = sf.read(SRC, always_2d=True); assert sr == SR
x = x[int(0.3 * SR):]                                   # skip the very first attack of the take
NAMES = ['C', 'Cs', 'D', 'Ds', 'E', 'F', 'Fs', 'G', 'Gs', 'A', 'As', 'B']
lines = []; wavs = []
for R in range(36, 85, 3):
    ratio = 2 ** (((R - ROOT) - SRC_CENTS / 100) / 12)  # >1 = faster/higher
    src_s = min(len(x) / SR, 50.0 * ratio)                # never more than 50 s once transposed
    seg = x[:int(src_s * SR)]
    y = soxr.resample(seg, SR, SR / ratio, quality='VHQ')  # varispeed: played back at SR it sounds `ratio` higher
    fc = 4500.0 if R <= 64 else 4500.0 * 2 ** (-(R - 64) / 24)   # top zones a little darker still
    y = sosfiltfilt(butter(2, fc, 'low', fs=SR, output='sos'), y, axis=0)
    y = sosfiltfilt(butter(1, 30.0, 'high', fs=SR, output='sos'), y, axis=0)
    n = len(y); m = y.mean(1)
    cents = 0.0; rate = SR                               # varispeed is exact: the source's +4 cents are already in `ratio`
    xf = int(min(3.0, 0.2 * n / SR) * SR); ls = int(max(0.15 * n, xf + 0.5 * SR)); le = n - int(0.05 * SR)
    z = y[:le].copy(); g = np.linspace(0, 1, xf)[:, None]
    z[le - xf:le] = y[le - xf:le] * np.cos(g * np.pi / 2) + y[ls - xf:ls] * np.sin(g * np.pi / 2)
    lev = np.sqrt(np.mean(z.mean(1)[int(0.5 * SR):] ** 2)); scale = 0.98 / np.abs(z).max()
    pcm = np.clip(np.round(z * scale * 32767), -32768, 32767).astype('<i2')
    name = f'i22_wan_{R:03d}'
    pcm.tofile(os.path.join(RAW, name + '.raw'))
    sf.write(os.path.join(FLAC, name + '.flac'), pcm.reshape(-1, 2), SR, subtype='PCM_16')
    if WAV: sf.write(os.path.join(WAV, f'WA_Warm_{NAMES[R % 12]}{R // 12 - 1}.wav'), pcm.reshape(-1, 2), SR, subtype='PCM_16')
    gain = (0.12 / max(lev, 1e-6)) / scale
    for d, ext in ((RAW, 'raw'), (FLAC, 'flac')):
        lines.append((d, f'22 {R} 0 127 {len(z)} 2 {rate:.2f} {ls} {le} {gain:.6f} {name}.{ext}'))
    wavs.append((R, f'WA_Warm_{NAMES[R % 12]}{R // 12 - 1}.wav', ls, le, xf, cents))
    print(f'zone {R}: {len(z) / SR:5.1f} s, loop {ls / SR:.1f}-{le / SR:.1f} s, {cents:+.0f} cents corrected')
for d in (RAW, FLAC):
    for f in os.listdir(d):
        if f.startswith('i22_wa_'): os.remove(os.path.join(d, f))   # the old per-tonality takes
    p = os.path.join(d, 'samples.txt')
    old = [l for l in open(p) if not l.startswith('22 ')]
    open(p, 'w').write(''.join(old).rstrip('\n') + '\n' + '\n'.join(l for dd, l in lines if dd == d) + '\n')
if WAV:
    with open(os.path.join(WAV, 'zones.txt'), 'w') as fh:
        for w in wavs: fh.write(' '.join(str(v) for v in w) + '\n')
print('ok')
