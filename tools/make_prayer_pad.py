#!/usr/bin/env python3
"""The Prayer - an original warm worship pad, built note by note (one clean note per sample, nothing else).
Each sample: 5 band-limited saw voices at -6/-3/0/+3/+6 cents, each drifting slowly and independently (+-2 cents,
0.05-0.2 Hz) like an analog poly, so the chorus breathes instead of combing; a pure sine at the fundamental for body;
a soft triangle an octave up (-16 dB) for the 'glory'; a gentle 12 dB/oct spectral lowpass (no resonance) that
follows the key at half rate and breathes +-8 % at 0.07 Hz. Voices are spread across the stereo field.
One sample every 3 semitones from C1 to C7, 12 s each, loop 4-12 s with a 2.5 s equal-power crossfade.
Usage: make_prayer_pad.py RAW_BANK FLAC_BANK"""
import os, sys
import numpy as np, soundfile as sf
RAW, FLAC = sys.argv[1:3]
SR = 44100; DUR = 12.0; N = int(DUR * SR); t = np.arange(N) / SR
rng = np.random.default_rng(7)
def drift(rate, depth_c):
    ph = rng.uniform(0, 2 * np.pi); ph2 = rng.uniform(0, 2 * np.pi)
    return depth_c * (0.7 * np.sin(2 * np.pi * rate * t + ph) + 0.3 * np.sin(2 * np.pi * rate * 2.3 * t + ph2))
def voice(f0, cents, fc_t, wave='saw', partial_gain=1.0):
    r = 2 ** ((cents + drift(rng.uniform(0.05, 0.2), 2.0)) / 1200)
    phi = 2 * np.pi * np.cumsum(f0 * r) / SR
    out = np.zeros(N); kmax = int(min(9000, SR * 0.45) / f0)
    for k in range(1, kmax + 1):
        if wave == 'tri' and k % 2 == 0: continue
        a = (1 / k) if wave == 'saw' else (1 / (k * k))
        f = k * f0
        g = 1 / np.sqrt(1 + (f / fc_t) ** 4)          # 12 dB/oct, Q 0.7: no peak at all
        out += a * g * np.sin(k * phi + rng.uniform(0, 2 * np.pi))
    return out * partial_gain
lines = []
for R in range(24, 97, 3):
    f0 = 440 * 2 ** ((R - 69) / 12)
    fc = np.clip(1150 * 2 ** ((R - 60) / 24), 650, 3200)
    fc_t = fc * (1 + 0.08 * np.sin(2 * np.pi * 0.07 * t + rng.uniform(0, 6.28)))
    L = np.zeros(N); Rr = np.zeros(N)
    for c, pan in zip([-6, -3, 0, 3, 6], [-0.8, 0.4, 0.0, -0.4, 0.8]):
        v = voice(f0, c, fc_t)
        L += v * np.sqrt((1 - pan) / 2); Rr += v * np.sqrt((1 + pan) / 2)
    body = 0.9 * np.sin(2 * np.pi * np.cumsum(f0 * 2 ** (drift(0.08, 1.0) / 1200)) / SR)
    tri = voice(2 * f0, 0, fc_t * 1.4, 'tri', 0.16) if 2 * f0 < 4000 else 0
    L += body + tri; Rr += body + tri
    x = np.stack([L, Rr], 1)
    # loop: 4 s -> end, 2.5 s equal-power crossfade baked in
    ls, le, xf = int(4 * SR), N, int(2.5 * SR)
    z = x.copy(); g = np.linspace(0, 1, xf)[:, None]
    z[le - xf:le] = x[le - xf:le] * np.cos(g * np.pi / 2) + x[ls - xf:ls] * np.sin(g * np.pi / 2)
    z[:int(0.01 * SR)] *= np.linspace(0, 1, int(0.01 * SR))[:, None]
    lev = np.sqrt(np.mean(z.mean(1) ** 2)); scale = 0.98 / np.abs(z).max()
    pcm = np.clip(np.round(z * scale * 32767), -32768, 32767).astype('<i2')
    name = f'i22_pad_{R:03d}'
    pcm.tofile(os.path.join(RAW, name + '.raw'))
    sf.write(os.path.join(FLAC, name + '.flac'), pcm.reshape(-1, 2), SR, subtype='PCM_16')
    gain = (0.12 / max(lev, 1e-6)) / scale
    for d, ext in ((RAW, 'raw'), (FLAC, 'flac')):
        lines.append((d, f'22 {R} 0 127 {N} 2 {SR:.2f} {ls} {le} {gain:.6f} {name}.{ext}'))
    print('note', R, 'ok')
for d in (RAW, FLAC):
    for f in os.listdir(d):
        if f.startswith('i22_wa'): os.remove(os.path.join(d, f))   # the W-A takes are no longer used
    p = os.path.join(d, 'samples.txt')
    old = [l for l in open(p) if not l.startswith('22 ')]
    open(p, 'w').write(''.join(old).rstrip('\n') + '\n' + '\n'.join(l for dd, l in lines if dd == d) + '\n')
print('ok')
