#!/usr/bin/env python3
"""Prepare the VSCO 2 CE samples (CC0) used by D-SYNTH 2.

Trims, loops (crossfade baked in), levels and converts the source WAVs to raw int16 files
plus a manifest (samples.txt) that the engine reads.  Usage: prep_samples.py VSCO_DIR OUT_DIR
"""
import glob, os, re, sys
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

SRC, OUT = sys.argv[1], sys.argv[2]
os.makedirs(OUT, exist_ok=True)
SR = 44100
NOTE = {'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11}

# inst id, folder, kind (loop / shot), max seconds, stereo, keep velocity layers
INSTS = [
    (0,  'Strings/Violin Section/susVib',  'loop', 5.0, True),
    (1,  'Strings/Viola Section/susvib',   'loop', 5.0, True),
    (2,  'Strings/Cello Section/susvib',   'loop', 5.0, True),
    (3,  'Strings/Solo Contrabass/SusVib', 'loop', 5.0, True),
    (4,  'Brass/Trumpet/sus',              'loop', 4.0, False),
    (5,  'Brass/Trumpet/stac',             'shot', 1.2, False),
    (6,  'Brass/F Horn/sus',               'loop', 4.0, False),
    (7,  'Brass/F Horn/stac',              'shot', 1.2, False),
    (8,  'Brass/Tenor Trombone/sus',       'loop', 4.0, False),
    (9,  'Brass/Tenor Trombone/stac',      'shot', 1.2, False),
    (10, 'Brass/Tuba/sus',                 'loop', 4.0, False),
    (11, 'Brass/Tuba/stac',                'shot', 1.2, False),
]

def load(path):
    sr, x = wavfile.read(path)
    if x.dtype == np.int16: x = x / 32768.0
    elif x.dtype == np.int32: x = x / 2147483648.0
    else: x = x.astype(np.float64)
    if x.ndim == 1: x = x[:, None]
    if sr != SR: x = resample_poly(x, SR, sr, axis=0)
    return x

def trim_start(x):
    a = np.abs(x).max(1); thr = a.max() * 10 ** (-48 / 20)
    i = int(np.argmax(a > thr)); return x[max(0, i - int(0.002 * SR)):]

def rms_env(m, win):
    k = np.ones(win) / win
    return np.sqrt(np.convolve(m * m, k, mode='same') + 1e-12)

def make_loop(x, max_s):
    m = x.mean(1); env = rms_env(m, int(0.05 * SR)); n = len(m)
    s0, s1 = int(0.2 * n), int(0.6 * n)
    sus = np.median(env[s0:s1]) if s1 > s0 else env.max()
    ok = np.where(env > 0.6 * sus)[0]
    end = min(int(ok[-1]) if len(ok) else n - 1, int(max_s * SR), n - 1)
    xf = int(0.25 * SR)
    start_guess = max(int(0.35 * SR) + xf, end - int(2.2 * SR))
    # best loop start: match a short window before the end with a window before the start
    w = int(0.02 * SR); ref = m[end - w:end]
    best, bs = -1e9, start_guess
    for s in range(max(xf + w, start_guess - int(0.25 * SR)), min(end - int(0.8 * SR), start_guess + int(0.25 * SR)), 8):
        c = np.dot(ref, m[s - w:s]) / (np.linalg.norm(ref) * np.linalg.norm(m[s - w:s]) + 1e-12)
        if c > best: best, bs = c, s
    start = bs
    y = x[:end].copy()
    f = np.linspace(0, 1, xf)[:, None]
    g_out, g_in = np.cos(f * np.pi / 2), np.sin(f * np.pi / 2)
    y[end - xf:end] = x[end - xf:end] * g_out + x[start - xf:start] * g_in
    return y, start, end

def shot(x, max_s):
    n = min(len(x), int(max_s * SR)); y = x[:n].copy(); fl = min(int(0.15 * SR), n // 3)
    y[n - fl:] *= np.linspace(1, 0, fl)[:, None]; return y

lines = []
def emit(inst, root, vlo, vhi, y, ls, le, stereo, name):
    if not stereo: y = y.mean(1, keepdims=True)
    m = y.mean(1); body = m[int(0.05 * SR):] if len(m) > SR // 10 else m
    level = np.sqrt(np.mean(body ** 2)) if inst != 12 else np.abs(m).max() * 0.25
    gain = 0.12 / max(level, 1e-6)
    pk = np.abs(y).max(); scale = 0.98 / pk if pk > 0 else 1
    pcm = np.clip(np.round(y * scale * 32767), -32768, 32767).astype('<i2')
    fn = f'{name}.raw'; pcm.tofile(os.path.join(OUT, fn))
    rate = SR * (2 ** (-0.12 / 12) if inst == 17 else 1.0)
    lines.append(f'{inst} {root} {vlo} {vhi} {len(y)} {y.shape[1]} {rate:.2f} {ls} {le} {gain / scale:.6f} {fn}')

for inst, folder, kind, max_s, stereo in INSTS:
    files = sorted(glob.glob(os.path.join(SRC, folder, '*.wav')))
    by_note = {}
    for f in files:
        mm = re.search(r'_([A-G]#?)(-?\d)_v(\d)', os.path.basename(f))
        if not mm: continue
        if 'rr' in os.path.basename(f) and '_rr1' not in os.path.basename(f): continue
        midi = NOTE[mm.group(1)] + 12 * (int(mm.group(2)) + 2)   # VSCO names A3 = 440 Hz
        by_note.setdefault(midi, {})[int(mm.group(3))] = f
    for midi, layers in sorted(by_note.items()):
        vs = sorted(layers); pick = [(vs[0], 0, 84)] + ([(vs[-1], 85, 127)] if len(vs) > 1 else [])
        if len(pick) == 1: pick = [(vs[0], 0, 127)]
        for v, lo, hi in pick:
            x = trim_start(load(layers[v]))
            if kind == 'loop': y, ls, le = make_loop(x, max_s)
            else: y, ls, le = shot(x, max_s), 0, 0
            emit(inst, midi, lo, hi, y, ls, le, stereo, f'i{inst:02d}_{midi:03d}_v{v}')
    print(folder, len(by_note), 'notes')

# upright piano: key numbers from the mapping chart, 3 dynamics -> keep softest and loudest
pdir = os.path.join(SRC, 'Keys/Upright Piano')
chart = dict(l.strip().split('=') for l in open(os.path.join(pdir, 'MappingChart.txt')) if re.match(r'^\d{3}=', l.strip()))
dyns = sorted({int(re.search(r'dyn(\d)', f).group(1)) for f in glob.glob(os.path.join(pdir, 'Player_dyn*_rr1_*.wav'))})
for d, lo, hi in [(dyns[0], 0, 70), (dyns[len(dyns) // 2], 71, 104), (dyns[-1], 105, 127)]:
    for f in sorted(glob.glob(os.path.join(pdir, f'Player_dyn{d}_rr1_*.wav'))):
        idx = re.search(r'_(\d{3})\.wav', f).group(1); midi = int(chart[idx])
        y = shot(trim_start(load(f)), 7.0 if midi < 60 else 5.0)
        emit(12, midi, lo, hi, y, 0, 0, True, f'i12_{midi:03d}_d{d}')
print('piano dyns', dyns)

# Dan Malaba's organ recordings: one file per key, four registrations (mono, dry, no Leslie)
ORG = sys.argv[3] if len(sys.argv) > 3 else None
if ORG and os.path.isdir(ORG):
    for inst, reg in [(13, 'Gospel'), (14, '888862200'), (15, 'All 9 Bars'), (16, "J's Jazz")]:
        cnt = 0
        for midi in range(36, 97, 2):
            f = os.path.join(ORG, f'{reg} {midi:03d}.wav')
            if not os.path.exists(f): continue
            x = load(f)[int(0.002 * SR):]
            y, ls, le = make_loop(x, 3.2)
            emit(inst, midi, 0, 127, y, ls, le, False, f'i{inst:02d}_{midi:03d}')
            cnt += 1
        print('organ', reg, cnt, 'notes')

# glockenspiel (Versilian Community Sample Library, CC0): sounds one octave above its written name
VCSL = sys.argv[4] if len(sys.argv) > 4 else None
if VCSL:
    gdir = os.path.join(VCSL, 'Idiophones/Struck Idiophones/Glockenspiel')
    layers = {'soft': (0, 60), 'medium': (61, 100), 'loud': (101, 127)}
    for f in sorted(glob.glob(os.path.join(gdir, 'glock_*.wav'))):
        mm = re.search(r'glock_(soft|medium|loud)_([A-G]#?)(\d)_', os.path.basename(f))
        if not mm: continue
        midi = NOTE[mm.group(2)] + 12 * (int(mm.group(3)) + 2)
        lo, hi = layers[mm.group(1)]
        from scipy.signal import butter, sosfilt
        y = shot(trim_start(load(f)), 4.0)
        y = sosfilt(butter(2, 350, 'highpass', fs=SR, output='sos'), y, axis=0)   # no mallet thump under the bars
        emit(17, midi, lo, hi, y, 0, 0, False, f'i17_{midi:03d}_{mm.group(1)}')
    print('glockenspiel ok')

# Daniel Malaba's string ensemble ("Rich Strings"): files Rich_Strings_<note>_<velocity>.wav
STR = sys.argv[5] if len(sys.argv) > 5 else None
if STR and os.path.isdir(STR):
    cnt = 0
    for f in sorted(glob.glob(os.path.join(STR, 'Rich_Strings_*_*.wav'))):
        mm = re.search(r'Rich_Strings_(\d{3})_(\d{3})\.wav', f)
        midi, vel = int(mm.group(1)), int(mm.group(2))
        if vel not in (63, 109): continue
        lo, hi = (0, 84) if vel == 63 else (85, 127)
        x = load(f)
        m = x.mean(1); env = rms_env(m, int(0.02 * SR)); n = len(m)
        sus = np.median(env[int(0.3 * n):int(0.7 * n)])
        # remove the slow bow swell recorded in the sample: the engine's attack shapes the start instead
        thr = (0.3 if vel == 63 else 0.15) * sus
        st = max(0, int(np.argmax(env > thr)) - int(0.01 * SR))
        y = x[st:].copy(); fi = int(0.015 * SR); y[:fi] *= np.linspace(0, 1, fi)[:, None]
        y, ls, le = make_loop(y, 6.0)
        emit(18, midi, lo, hi, y, ls, le, True, f'i18_{midi:03d}_v{vel}')
        cnt += 1
    print('dan strings', cnt, 'zones')
    # second layer: "Film Strings Sustain <note>" (named one octave low: A3 = 440 Hz), adds air on top
    cnt = 0
    for f in sorted(glob.glob(os.path.join(STR, 'Film Strings Sustain *.wav'))):
        mm = re.search(r'Sustain ([A-G]#?)(-?\d)\.wav', f)
        if not mm: continue
        midi = NOTE[mm.group(1)] + 12 * (int(mm.group(2)) + 2)
        if midi < 36 or midi > 96: continue
        x = load(f); m = x.mean(1); env = rms_env(m, int(0.02 * SR)); n = len(m)
        sus = np.median(env[int(0.3 * n):int(0.7 * n)])
        st = max(0, int(np.argmax(env > 0.25 * sus)) - int(0.01 * SR))
        y = x[st:].copy(); fi = int(0.015 * SR); y[:fi] *= np.linspace(0, 1, fi)[:, None]
        y, ls, le = make_loop(y, 6.0)
        emit(19, midi, 0, 127, y, ls, le, True, f'i19_{midi:03d}')
        cnt += 1
    print('film strings', cnt, 'zones')

with open(os.path.join(OUT, 'samples.txt'), 'w') as fh:
    fh.write('# D-SYNTH 2 samples: inst root velLo velHi frames channels rate loopStart loopEnd gain file\n')
    fh.write('# Sources: VS Chamber Orchestra Community Edition and Versilian Community Sample Library (CC0 1.0),\n')
    fh.write('#          organ and string recordings provided by Daniel Malaba.\n')
    fh.write('\n'.join(lines) + '\n')
print(len(lines), 'zones')
