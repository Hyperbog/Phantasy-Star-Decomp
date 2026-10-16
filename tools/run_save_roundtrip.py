#!/usr/bin/env python3
"""Two-process in-game SAVE/CONTINUE native-vs-interpreter verification.

Each mode starts with EMPTY cartridge SRAM, navigates the ORIGINAL ROM's
SAVE menu, writes a named character to a slot, terminates the process, then
starts a fresh process and selects CONTINUE -> YES -> that slot.

Evidence is private. The same-runtime interpreter is NOT an independent oracle.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

from run_scenario import ROOT, SHA256, compare, read_csv, scenarios

SAVE_FRAMES = 9150
CONTINUE_FRAMES = 2300


def save_events(slot):
    if slot not in (1, 2):
        raise ValueError("only verified save slots 1 and 2 are supported")
    events = list(scenarios()['interactions6200'][1])
    # At frame 6200 the STAS/MAGC/ITEM/SRCH/SAVE menu is awaiting input.
    for frame in (6230, 6280, 6330, 6380):
        events += [(frame, 'D'), (frame + 7, '')]
    # SAVE -> acknowledge prompt -> choose slot -> YES.
    events += [(6530, 'B'), (6538, ''), (6800, 'B'), (6808, '')]
    if slot == 2:
        events += [(6860, 'D'), (6868, '')]
    events += [(6930, 'B'), (6938, ''), (7250, 'B'), (7260, '')]
    # The unused slot requests a character name. Select 'A', move to END.
    events += [(7700, 'B'), (7708, '')]
    for j, key in enumerate('DDDRRRRRRR'):
        frame = 7810 + 36*j
        events += [(frame, key), (frame + 7, '')]
    events += [(8450, 'B'), (8460, '')]
    return events


def continue_events(slot):
    if slot not in (1, 2):
        raise ValueError("only verified save slots 1 and 2 are supported")
    # Start screen -> CONTINUE -> YES -> numbered slot.
    events = [(310, 'D'), (318, ''), (370, 'B'), (378, ''),
              (1400, 'B'), (1408, '')]
    if slot == 2:
        events += [(1580, 'D'), (1588, '')]
    events += [(1660, 'B'), (1668, '')]
    return events


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def transitions(rows):
    return [[i+1, row['mode_c202']]
            for i, row in enumerate(rows)
            if i == 0 or row['mode_c202'] != rows[i-1]['mode_c202']]


def invoke(exe, rom, folder, mode, phase, frames, events, savefile):
    folder.mkdir(parents=True, exist_ok=True)
    prefix = folder / phase
    cmd = [str(exe), str(rom), '--frames', str(frames),
           '--sram-file', str(savefile),
           '--vdp-trace', str(folder / f'{phase}.csv'),
           '--psg-log', str(folder / f'{phase}.psg.csv'),
           '--dump-frame', str(frames), '--dump-out', str(folder / f'{phase}.png')]
    if mode == 'interpreter':
        cmd += ['--interp']
    for frame, keys in events:
        cmd += ['--press', f'{frame}:{keys}']
    with (folder / f'{phase}.stderr.txt').open('w') as f:
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=f,
                       cwd=folder, timeout=180)
    rows = read_csv(folder / f'{phase}.csv')
    if len(rows) != frames:
        raise ValueError(f'{phase} {mode}: incomplete VDP trace: {len(rows)}/{frames}')
    return rows


def check_pair(output, phase, frames, failures):
    a = output / 'native' / phase
    b = output / 'interpreter' / phase
    # Reuse the existing fail-closed per-frame and audio parity verifier.
    # Its layout is two files at the same level; compare without copying ROMs.
    pair = output / f'{phase}_pair'
    pair.mkdir()
    for mode, origin in [('native', a), ('interpreter', b)]:
        for ext in ('.csv', '.png', '.psg.csv'):
            shutil.copyfile(origin.with_suffix(ext) if ext != '.psg.csv'
                            else origin.with_name(origin.name + '.psg.csv'),
                            pair / f'{mode}{ext}')
    result = compare(pair, frames)
    failures += [f'{phase}: {e}' for e in result['errors']]
    # Compare full endpoint snapshots, not just a hash or final pixels.
    result['snapshot_differences'] = {}
    for ext in ('ram', 'vram', 'cram', 'regs'):
        na = (output / 'native' / f'{phase}.png.{ext}').read_bytes()
        nb = (output / 'interpreter' / f'{phase}.png.{ext}').read_bytes()
        differences = [k for k, (x,y) in enumerate(zip(na, nb)) if x != y]
        result['snapshot_differences'][ext] = {
            'different_bytes': len(differences) + abs(len(na) - len(nb)),
            'first_different_offset': differences[0] if differences else None}
        if ext in ('vram', 'cram', 'regs') and na != nb:
            failures.append(f'{phase}: final {ext} differs')
    return result


def run_roundtrip(rom, out, slot, exe=None):
    if slot not in (1,2):
        raise ValueError('only explicitly exercised slots 1 and 2 are supported')
    if out.exists():
        raise FileExistsError(f'refusing to reuse existing evidence directory: {out}')
    if not rom.is_file() or sha256(rom.read_bytes()) != SHA256:
        raise ValueError('ROM must be the exact English Rev. 3 SHA-256')
    exe = exe or (ROOT / 'bin/PhantasyStarNative')
    if not exe.is_file():
        raise FileNotFoundError(f'build the native executable first: {exe}')
    out.mkdir(parents=True)
    failures = []
    mode_details = {}
    for mode in ('native', 'interpreter'):
        folder = out / mode
        savefile = folder / 'cartridge.sav'
        sr = invoke(exe,rom,folder,mode,'save',SAVE_FRAMES,save_events(slot),savefile)
        mem = savefile.read_bytes()
        if len(mem) != 0x8000 or not mem.startswith(b'PHANTASY STAR'):
            failures.append(f'{mode}: cartridge save absent/invalid')
            continue
        if mem[0x200 + slot] != 1:
            failures.append(f'{mode}: in-game slot {slot} not marked occupied')
        payload = mem[(slot)*0x400 : (slot+1)*0x400]
        # Must be an actual game-state copy, not just the SRAM backup header.
        if payload == bytes(1024) or payload == b'\xff'*1024:
            failures.append(f'{mode}: slot {slot} contains no gameplay payload')
        st=transitions(sr)
        if not ([7311, '10'] in st and [7347, '11'] in st and
                [8451, '0C'] in st and [8507, '0D'] in st):
            failures.append(f'{mode}: expected save/name-entry game transitions missing: {st[-6:]}')
        before=sha256(mem)
        cr=invoke(exe,rom,folder,mode,'continue',CONTINUE_FRAMES,continue_events(slot),savefile)
        after=sha256(savefile.read_bytes())
        if before != after:
            failures.append(f'{mode}: CONTINUE unexpectedly changed cartridge save bytes')
        ct=transitions(cr)
        if [1760,'09'] not in ct or cr[-1]['mode_c202'] != '09':
            failures.append(f'{mode}: saved game was not restored into map mode: {ct}')
        ram=(folder/'continue.png.ram').read_bytes()
        if len(ram) != 8192:
            failures.append(f'{mode}: invalid 8 KiB work RAM dump')
        else:
            # C300..C6FF is the game's actual 1 KiB save payload. The first
            # byte (map movement-state flag) is changed during map loading.
            payload_mismatches=[j for j in range(1,0x400)
                                if ram[0x300+j] != payload[j]]
            if payload_mismatches:
                failures.append(f'{mode}: saved C301..C6FF not restored: first offset {payload_mismatches[0]+0x300:04X}')
        mode_details[mode]={
            'slot_flag':mem[0x200+slot],
            'slot_payload_sha256':sha256(payload),
            'save_sha256':before,
            'save_unchanged_after_continue':before==after,
            'save_transitions':st[-8:],
            'continue_transitions':ct[-8:],
            'restored_payload_compared_bytes':0x3ff,
            'restored_payload_mismatches':len(payload_mismatches) if len(ram)==8192 else None}

    if all((out / m / 'cartridge.sav').is_file() for m in ('native','interpreter')):
        same = (out/'native/cartridge.sav').read_bytes()==(out/'interpreter/cartridge.sav').read_bytes()
        if not same:failures.append('cartridge save image differs between native and interpreter')
    else:same=False
    # Negative control: the identical CONTINUE inputs must NOT resume gameplay
    # when the selected slot is empty. An accepted continue despite an empty
    # slot would make a screenshot-only round-trip an invalid proof of save.
    for mode in ('native', 'interpreter'):
        folder = out/mode
        erased=folder/'empty_cartridge.sav'
        erased.write_bytes(b'\xff'*0x8000)
        rows=invoke(exe,rom,folder,mode,'empty',CONTINUE_FRAMES,continue_events(slot),erased)
        if rows[-1]['mode_c202'] != '03' or any(x['mode_c202'] == '09' for x in rows):
            failures.append(f'{mode}: an empty SAVE slot incorrectly resumed gameplay')
        if erased.read_bytes()[0x200+slot] != 0:
            failures.append(f'{mode}: negative-control slot was unexpectedly populated')
    if (out/'native/empty_cartridge.sav').read_bytes() != (out/'interpreter/empty_cartridge.sav').read_bytes():
        failures.append('negative-control SRAM initializations differ')
    pairs = {}
    if not failures:
        for phase, frames in (('save', SAVE_FRAMES), ('continue', CONTINUE_FRAMES),
                              ('empty', CONTINUE_FRAMES)):
            pairs[phase]=check_pair(out,phase,frames,failures)
    result={
        'ok':not failures, 'slot':slot, 'save_frames':SAVE_FRAMES,
        'continue_frames':CONTINUE_FRAMES, 'rom_sha256':SHA256,
        'fresh_process_for_continue':True,
        'cartridge_sram_identical':same,
        'modes':mode_details, 'parity':pairs,
        'errors':failures,
        'reference_type':'bundled superzazu interpreter, not independent emulator'}
    (out/'result.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path)
    p.add_argument('--slot',type=int,choices=(1,2),default=1)
    args=p.parse_args()
    try:
        result=run_roundtrip(args.rom.resolve(),args.out.resolve(),args.slot)
    except (ValueError,FileNotFoundError,FileExistsError,subprocess.SubprocessError) as e:
        p.error(str(e))
    print(json.dumps({k:v for k,v in result.items() if k!='parity'}, indent=2))
    return 0 if result['ok'] else 1

if __name__=='__main__':
    raise SystemExit(main())
