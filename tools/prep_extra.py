#!/usr/bin/env python3
"""Adds Daniel Malaba's horn sections (Glory Brass) and his W-A pads (The Prayer) to a D-SYNTH sample bank.
Usage: prep_extra.py BANK_DIR BRASS_DIR WA_DIR   (BRASS_DIR: Pop_Horns_<vel>_<note>.flac...; WA_DIR: WA_<key>.flac)
Writes raw int16 files into BANK_DIR and replaces those instruments' lines in samples.txt."""
import glob, os, re, sys
import numpy as np, soundfile as sf
from scipy.signal import resample_poly
BANK, BRASS, WA = sys.argv[1:4]
SR = 44100
def load(p):
    x, sr = sf.read(p, always_2d=True)
    if sr != SR: x = resample_poly(x, SR, sr, axis=0)
    return x
def rms_env(m, win): return np.sqrt(np.convolve(m * m, np.ones(win) / win, mode='same') + 1e-12)
def trim_start(x):
    a = np.abs(x).max(1); thr = a.max() * 10 ** (-48 / 20); i = int(np.argmax(a > thr)); return x[max(0, i - int(0.002 * SR)):]
def make_loop(x, max_s, xf_s=0.25, min_len=0.8):
    m = x.mean(1); env = rms_env(m, int(0.05 * SR)); n = len(m)
    sus = np.median(env[int(0.2 * n):int(0.6 * n)])
    ok = np.where(env > 0.6 * sus)[0]
    end = min(int(ok[-1]) if len(ok) else n - 1, int(max_s * SR), n - 1)
    xf = int(xf_s * SR); guess = max(int(0.35 * SR) + xf, end - int(2.2 * SR)); w = int(0.02 * SR); ref = m[end - w:end]
    best, bs = -1e9, guess
    for s in range(max(xf + w, guess - int(0.25 * SR)), min(end - int(min_len * SR), guess + int(0.25 * SR)), 8):
        c = np.dot(ref, m[s - w:s]) / (np.linalg.norm(ref) * np.linalg.norm(m[s - w:s]) + 1e-12)
        if c > best: best, bs = c, s
    y = x[:end].copy(); f = np.linspace(0, 1, xf)[:, None]
    y[end - xf:end] = x[end - xf:end] * np.cos(f * np.pi / 2) + x[bs - xf:bs] * np.sin(f * np.pi / 2)
    return y, bs, end
lines = []
def emit(inst, root, vlo, vhi, y, ls, le, name, level_ref=None, rate=SR):
    m = y.mean(1); body = m[int(0.05 * SR):]
    level = level_ref if level_ref else np.sqrt(np.mean(body ** 2))
    gain = 0.12 / max(level, 1e-6); pk = np.abs(y).max(); scale = 0.98 / pk
    pcm = np.clip(np.round(y * scale * 32767), -32768, 32767).astype('<i2')
    fn = f'{name}.raw'; pcm.tofile(os.path.join(BANK, fn))
    lines.append(f'{inst} {root} {vlo} {vhi} {len(y)} {y.shape[1]} {rate:.2f} {ls} {le} {gain / scale:.6f} {fn}')

# horn sections: 20 = Pop Horns (punch), 21 = Mellow (warm body). Velocity layers keep their own loudness
# relative to the loudest layer (a soft horn is quieter, not just darker).
for inst, pat, vels in [(20, 'Pop_Horns', [(65, 0, 79), (80, 80, 99), (110, 100, 117), (127, 118, 127)]),
                        (21, 'Mellow', [(75, 0, 84), (95, 85, 104), (115, 105, 119), (127, 120, 127)])]:
    cnt = 0
    notes = sorted({int(re.search(r'_(\d{3})\.flac$', f).group(1)) for f in glob.glob(os.path.join(BRASS, f'{pat}_127_*.flac'))})
    for note in notes:
        ref = None; zones = []
        for v, lo, hi in vels:
            cand = sorted(glob.glob(os.path.join(BRASS, f'{pat}_{v:03d}_*.flac')), key=lambda f: abs(int(f[-8:-5]) - note))
            if not cand: continue
            f = cand[0]; root = int(f[-8:-5])
            if abs(root - note) > 1: continue
            x = trim_start(load(f)); y, ls, le = make_loop(x, 6.0)
            lev = np.sqrt(np.mean(y.mean(1)[int(0.05 * SR):] ** 2)); zones.append((v, lo, hi, root, y, ls, le, lev))
        if not zones: continue
        top = max(z[7] for z in zones)
        for v, lo, hi, root, y, ls, le, lev in zones:
            # same gain for every layer of a note (normalised on the loudest): dynamics come from the recordings
            emit(inst, root, lo, hi, y, ls, le, f'i{inst}_{root:03d}_v{v}', level_ref=top)
            cnt += 1
    print(pat, cnt, 'zones')

# W-A pads: one 75 s take per key (root + fifth), long loop with a 3 s crossfade
KEYS = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
for k, name in enumerate(KEYS):
    f = os.path.join(WA, f'WA_{name}.flac')
    if not os.path.exists(f): continue
    x = load(f); n = len(x); xf = int(3.0 * SR); ls = int(6.0 * SR); le = n - int(0.5 * SR)
    import librosa
    cents = 100.0 * float(librosa.estimate_tuning(y=librosa.resample(x.mean(1)[:60 * SR], orig_sr=SR, target_sr=22050), sr=22050))
    tune_rate = SR * 2 ** (-cents / 1200.0)   # played back slightly faster/slower so every key sits on A = 440 Hz
    print(f'WA {name}: {cents:+.0f} cents corrected')
    y = x[:le].copy(); g = np.linspace(0, 1, xf)[:, None]
    y[le - xf:le] = x[le - xf:le] * np.cos(g * np.pi / 2) + x[ls - xf:ls] * np.sin(g * np.pi / 2)
    emit(22, 60 + k, 0, 127, y, ls, le, f'i22_wa_{k:02d}', rate=tune_rate)
print('W-A pads ok')

old = [l for l in open(os.path.join(BANK, 'samples.txt')) if not re.match(r'^(20|21|22) ', l)]
with open(os.path.join(BANK, 'samples.txt'), 'w') as fh:
    fh.write(''.join(old).rstrip('\n') + '\n' + '\n'.join(lines) + '\n')
print(len(lines), 'zones added')
