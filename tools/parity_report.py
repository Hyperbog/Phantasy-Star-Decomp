#!/usr/bin/env python3
"""Report observed native / SAME-RUNTIME interpreter agreement by frame.
Does not claim independent hardware or emulator verification.
"""
import csv
from pathlib import Path
P=Path(__file__).resolve().parents[1]/'evidence'
with (P/'native_frames_600.csv').open(newline='') as f:n=list(csv.DictReader(f))
with (P/'interpreter_frames_600.csv').open(newline='') as f:i=list(csv.DictReader(f))
assert len(n)==len(i)==600
for k in ('vram_h','cram_h','reg_h','ram_h','mode_c202','wait_c208'):
    ds=[idx+1 for idx,(a,b) in enumerate(zip(n,i)) if a[k]!=b[k]]
    print(f'{k:12} different={len(ds):3} first={ds[0] if ds else "none":>4} matching@300={n[299][k]==i[299][k]} matching@600={n[599][k]==i[599][k]}')
