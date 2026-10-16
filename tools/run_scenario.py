#!/usr/bin/env python3
"""Fail-closed native / same-runtime interpreter replay. No ROM copied or bundled."""
import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SHA256 = '9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88'
FIELDS = ('mode_c202','wait_c208','timer_c21b','timer_c21d',
          'anim_c264','anim_c307','cram_h','reg_h','r8','r9','r0','r1')


def scenarios():
    intro = [(310, 'A'), (315, '')]
    for frame in (1870, 2150, 2430, 2710, 2990, 3270, 3550, 3830):
        intro.extend(((frame, 'A'), (frame + 6, '')))
    interactions = list(intro)
    for frame in (4110, 4310, 4590, 4870, 5150, 5430, 5710):
        interactions.extend(((frame, 'A'), (frame + 6, '')))
    movement = list(intro) + [(3900,'U'),(4440,''),(4680,'R'),(5200,'')]
    # This replay enters Alis's home and advances several dialogue/menu turns.
    # It then navigates back to the overhead town view.
    city_navigation = list(intro) + [(3900,'U')]
    for frame in (4100,4400,4700,5000,5400,5800):
        city_navigation.extend(((frame,'A'),(frame+10,'')))
    city_navigation.append((6200,'U'))
    for frame in (9010,9080,9150,9220):
        city_navigation.extend(((frame,'D'),(frame+8,'')))
    city_navigation.extend(((9340,'A'),(9348,'')))
    city_south = city_navigation + [(9750,'D'),(14490,'')]
    return {'intro600':(600,[(310,'A'),(315,'')]),
            'intro1800':(1800,[(310,'A'),(315,'')]),
            'interaction4200':(4200,intro+[(4110,'A'),(4116,'')]),
            'interactions6200':(6200,interactions),
            'movement5400':(5400,movement),
            'city9700':(9700,city_navigation),
            'citysouth14500':(14500,city_south)}


def read_csv(path):
    with path.open(newline='') as f:
        return list(csv.DictReader(f))


def compare(prefix, frames):
    n, i = read_csv(prefix/'native.csv'),read_csv(prefix/'interpreter.csv')
    errors = []
    if len(n) != frames or len(i) != frames:
        errors.append(f'frame count {len(n)}/{len(i)}, expected {frames}')
    diffs = {}
    for key in FIELDS + ('vram_h','ram_h','cycles'):
        mismatch = [j+1 for j,(x,y) in enumerate(zip(n,i)) if x[key] != y[key]]
        diffs[key] = {'count':len(mismatch), 'first':mismatch[0] if mismatch else None}
        if key in FIELDS and mismatch:
            errors.append(f'{key}: first disagreement frame {mismatch[0]}')
    images_match=(prefix/'native.png').read_bytes() == (prefix/'interpreter.png').read_bytes()
    if not images_match: errors.append('final rendered PNG differs')
    if n and i and n[-1]['cycles'] != i[-1]['cycles']:
        errors.append('effective CPU clock differs at final frame')
    psg = None
    if not (prefix/'native.psg.csv').is_file() or not (prefix/'interpreter.psg.csv').is_file():
        errors.append('PSG register-write trace missing')
    else:
        pn, pi = read_csv(prefix/'native.psg.csv'), read_csv(prefix/'interpreter.psg.csv')
        values_match = [x['val'] for x in pn] == [x['val'] for x in pi]
        first_clock_diff = next((j+1 for j,(x,y) in enumerate(zip(pn,pi))
                                 if x['cyc']!=y['cyc']),None)
        if not values_match:
            errors.append('PSG register-write sequence differs')
        psg = {'writes_native':len(pn),'writes_reference':len(pi),
               'values_match':values_match,
               'cycle_timestamps_exact':first_clock_diff is None and len(pn)==len(pi),
               'first_timestamp_difference_write':first_clock_diff}
    return {'ok':not errors,'frames':frames,'errors':errors,
            'images_match':images_match,'fields':diffs,'psg':psg}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--rom',required=True,type=Path)
    ap.add_argument('--scenario',choices=tuple(scenarios()),required=True)
    ap.add_argument('--out',type=Path,default=ROOT/'runs')
    ap.add_argument('--with-sram',action='store_true',
                    help='start with erased cartridge SRAM in both modes and compare persisted images')
    args = ap.parse_args()
    rom = args.rom.resolve()
    if not rom.is_file() or hashlib.sha256(rom.read_bytes()).hexdigest() != SHA256:
        ap.error('ROM missing or SHA256 not identical to English Rev.3')
    exe=ROOT/'bin/PhantasyStarNative'
    if not exe.is_file(): ap.error(f'executable not found: {exe}; run bash build.sh')
    frames, presses=scenarios()[args.scenario]
    out=(args.out/args.scenario).resolve()
    out.mkdir(parents=True,exist_ok=True)
    if args.with_sram:
        for mode in ('native','interpreter'):
            if (out/f'{mode}.sav').exists():
                ap.error(f'refusing to overwrite existing {mode} SRAM; select a fresh --out directory')
    base=[str(exe),str(rom),'--frames',str(frames)]
    for frame, keys in presses: base.extend(('--press',f'{frame}:{keys}'))
    for mode in ('native','interpreter'):
        argv=base + (['--interp'] if mode=='interpreter' else []) + (
            ['--sram-file',str(out/f'{mode}.sav')] if args.with_sram else []) + [
            '--vdp-trace',str(out/f'{mode}.csv'), '--dump-frame',str(frames),
            '--dump-out',str(out/f'{mode}.png'),
            '--psg-log',str(out/f'{mode}.psg.csv')]
        with (out/f'{mode}.stderr.txt').open('w') as err:
            subprocess.run(argv,check=True,stderr=err,stdout=subprocess.DEVNULL,cwd=out)
    report=compare(out,frames)
    if args.with_sram:
        n=(out/'native.sav'); i=(out/'interpreter.sav')
        if not n.is_file() or not i.is_file():
            report['errors'].append('expected cartridge SRAM image not written')
        else:
            nb=n.read_bytes(); ib=i.read_bytes()
            if nb != ib: report['errors'].append('native/interpreter cartridge SRAM differs')
            if len(nb)!=0x8000 or len(ib)!=0x8000:
                report['errors'].append('unexpected SRAM image size')
            if not nb.startswith(b'PHANTASY STAR'):
                report['errors'].append('Phantasy Star backup RAM header missing')
            report['sram']={'size_native':len(nb), 'size_reference':len(ib),
                            'identical':nb==ib, 'game_backup_header':nb.startswith(b'PHANTASY STAR'),
                            'non_erased_bytes':sum(b!=0xff for b in nb)}
        report['ok'] = not report['errors']
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'scenario':args.scenario,**report},indent=2))
    return 0 if report['ok'] else 1


if __name__=='__main__':
    raise SystemExit(main())
