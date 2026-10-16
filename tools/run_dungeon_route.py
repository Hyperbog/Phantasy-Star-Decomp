#!/usr/bin/env python3
"""Stage 12: verified SAVE -> CONTINUE -> enter dungeon -> move seven tiles.

This is a *real* input-driven ROM replay, not a fabricated game state. Every
checkpoint restarts the executable with the same cartridge save image, so its
final RAM and graphics snapshots are fresh and cannot be recycled from an
older run. The comparator is same-runtime interpreter vs generated native C;
this is not independent hardware/Mesen accuracy verification.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

from run_scenario import ROOT, SHA256, FIELDS, compare, read_csv
from run_save_roundtrip import run_roundtrip, continue_events

# Snapshot after the left turn, after each forward step, and at a stable end.
# The game holds its wait selector $C208 across some frame boundaries; do not
# mistake that transient wait state for a permanently blocked game.
CHECKPOINTS = (
    (3120, 0x1E), (3250, 0x1D), (3380, 0x1C),
    (3510, 0x1B), (3640, 0x1A), (3770, 0x19),
    (3900, 0x18), (4030, 0x17), (4300, 0x17),
)
DUNGEON_DIRECTION = 3  # left from starting facing 0
DUNGEON_INDEX = 2      # the actual entrance reached by this saved-game route
EXPECTED_MODE_TRANSITIONS = ((1, '00'), (6, '02'), (39, '03'),
                             (409, '08'), (1760, '09'),
                             (2399, '0A'), (2434, '0B'))


def route_events(slot, actions='LUUUUUUU'):
    """Actual ROM controller input: restore slot, reach cave, then move."""
    if not actions or len(actions)>32 or any(key not in 'UDLR' for key in actions):
        raise ValueError('route must be 1..32 directional inputs (UDLR)')
    presses = list(continue_events(slot))
    presses += [(1950, 'R'), (2350, 'U'), (2600, '')]
    for n,key in enumerate(actions):
        frame = 3000 + 130*n
        presses += [(frame, key), (frame+8, '')]
    if any(a>=b for (a,_),(b,_) in zip(presses, presses[1:])):
        raise AssertionError('replay input directives must be strictly ordered')
    return presses


# An out-and-back corridor route tests both facing changes and map-coordinate
# updates; a separate south route tests another axis of dungeon navigation.
ALTERNATE_ROUTES = (
    ('west_return', 'LUUUUUUURRUUUUUUU', 5470, 0x1E, 1),
    ('south_three', 'DDD', 3650, 0x4E, 0),
    ('south_return', 'DDDUUU', 4040, 0x1E, 0),
)


def transition_log(rows):
    return [(i+1,r['mode_c202']) for i,r in enumerate(rows)
            if i==0 or r['mode_c202']!=rows[i-1]['mode_c202']]


def read_dungeon_state(ram):
    if len(ram)!=0x2000:
        raise ValueError(f'expected exactly 8192 bytes of RAM, got {len(ram)}')
    return {'game_mode':ram[0x202], 'direction':ram[0x30a],
            'position':ram[0x30c], 'dungeon_index':ram[0x30d],
            'enemy_id':ram[0x2e6]}


def verify_progress(states, checkpoint_positions=CHECKPOINTS):
    errors=[]
    if len(states)!=len(checkpoint_positions):
        return [f'checkpoint count incorrect: {len(states)} instead of {len(checkpoint_positions)}']
    for ((frame,position),state) in zip(checkpoint_positions,states):
        for field,want in [('game_mode',0x0b), ('direction',DUNGEON_DIRECTION),
                           ('position',position), ('dungeon_index',DUNGEON_INDEX)]:
            if state[field]!=want:
                errors.append(f'frame {frame}: {field} is {state[field]:02X}, expected {want:02X}')
    moved=sum(x['position'] != y['position'] for x,y in zip(states, states[1:]))
    if moved != 7:
        errors.append(f'only {moved} documented dungeon tile changes; expected 7')
    return errors


def compare_memory(folder, frame, failures):
    """Require exact video memory; allow only clearly identified stack drift."""
    record={}
    for ext in ('vram','cram','regs','ram'):
        a=(folder/f'native_{frame}.png.{ext}').read_bytes()
        b=(folder/f'interpreter_{frame}.png.{ext}').read_bytes()
        differences=[j for j in range(min(len(a),len(b))) if a[j]!=b[j]]
        if len(a)!=len(b):
            failures.append(f'{folder.name}: mismatched {ext} memory lengths')
        record[ext]={'different_bytes':len(differences),
                     'first_difference':differences[0] if differences else None}
        if ext!='ram' and differences:
            failures.append(f'{folder.name}: nonidentical final {ext} snapshot')
        if ext=='ram':
            # The original game executes interrupts on a stack in CA00-CAFF.
            # The Stage 9-11 hybrid may differ on stack contents even when
            # game-state RAM is identical; never silently exempt other RAM.
            unexpected=[i for i in differences if not (0xA00<=i<0xB00)]
            if unexpected:
                failures.append(f'{folder.name}: non-stack RAM differs at C{unexpected[0]:03X}')
            record[ext]['stack_only'] = not unexpected
    return record


def invoke(exe,rom,root,mode,frame,save_path,events):
    prefix = root / f'{mode}_{frame}'
    args=[str(exe),str(rom),'--frames',str(frame),'--sram-file',str(save_path),
          '--vdp-trace',str(prefix.with_suffix('.csv')),
          '--psg-log',str(root/f'{mode}_{frame}.psg.csv'),
          '--dump-frame',str(frame),'--dump-out',str(prefix.with_suffix('.png'))]
    if mode=='interpreter':args.append('--interp')
    for n,keys in events:
        if n <= frame:
            args += ['--press',f'{n}:{keys}']
    with (root/f'{mode}_{frame}.stderr.txt').open('w') as err:
        subprocess.run(args,check=True,cwd=root,stdout=subprocess.DEVNULL,
                       stderr=err,timeout=180)
    rows=read_csv(prefix.with_suffix('.csv'))
    if len(rows)!=frame:
        raise ValueError(f'{mode} frame {frame}: incomplete trace ({len(rows)} lines)')
    image=prefix.with_suffix('.png')
    state=read_dungeon_state((root/f'{mode}_{frame}.png.ram').read_bytes())
    return rows,state,image


def run(rom,out,slot=1,exe=None):
    rom=Path(rom).resolve();out=Path(out).resolve()
    if out.exists():raise FileExistsError(f'refusing to reuse evidence path {out}')
    if not rom.is_file() or hashlib.sha256(rom.read_bytes()).hexdigest()!=SHA256:
        raise ValueError('exact English Rev 3 ROM SHA-256 required')
    exe=Path(exe).resolve() if exe else ROOT/'bin/PhantasyStarNative'
    if not exe.is_file():raise FileNotFoundError(f'compiled executable absent: {exe}')
    if slot not in (1,2):raise ValueError('only previously verified SAVE slots 1 and 2')
    out.mkdir(parents=True)
    setup=run_roundtrip(rom,out/'save_restore_prerequisite',slot,exe=exe)
    failures=[]
    if not setup['ok']:
        failures+=['Stage 11 SAVE/CONTINUE prerequisite failed']+setup['errors']
    result={'ok':False, 'rom_sha256':SHA256, 'slot':slot,
            'oracle':'same-runtime superzazu (not independent Mesen)',
            'checkpoints':[], 'errors':failures,
            'prerequisite_ok':setup['ok']}
    if failures:
        (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        return result
    base=out/'dungeon';base.mkdir()
    inputs=route_events(slot)
    saved={}
    for mode in ('native','interpreter'):
        orig=out/'save_restore_prerequisite'/mode/'cartridge.sav'
        saved[mode]=orig.read_bytes()
        if len(saved[mode])!=0x8000 or saved[mode][0x200+slot]!=1:
            failures.append(f'{mode}: SAVE prerequisite contains no selected slot')
    if saved['native'] != saved['interpreter']:
        failures.append('native and reference source cartridges differ')
    if failures:
        result['errors']=failures
        (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        return result
    collected={'native':[],'interpreter':[]}
    for frame,position in CHECKPOINTS:
        record={'frame':frame,'expected_position':position}
        pair_dir=base/f'frame_{frame}';pair_dir.mkdir()
        for mode in ('native','interpreter'):
            sav=pair_dir/f'{mode}.sav'
            sav.write_bytes(saved[mode])   # independent fresh-process restart
            rows,state,image=invoke(exe,rom,pair_dir,mode,frame,sav,inputs)
            collected[mode].append(state)
            record[mode]=state
            if sav.read_bytes()!=saved[mode]:
                failures.append(f'{mode}, frame {frame}: dungeon traversal altered persistent SRAM')
            transitions=transition_log(rows)
            if transitions!=list(EXPECTED_MODE_TRANSITIONS):
                failures.append(f'{mode}, frame {frame}: incorrect game-mode sequence {transitions}')
            # The shared comparator demands PSG order, exact final frame, and
            # tracked mode/timer fields; it reports VRAM and RAM differences.
            shutil.copyfile(pair_dir/f'{mode}_{frame}.csv',pair_dir/f'{mode}.csv')
            shutil.copyfile(pair_dir/f'{mode}_{frame}.psg.csv',pair_dir/f'{mode}.psg.csv')
            shutil.copyfile(image,pair_dir/f'{mode}.png')
        comparison=compare(pair_dir,frame)
        record['parity']={'ok':comparison['ok'],
                          'image_identical':comparison['images_match'],
                          'psg_write_values_identical':comparison['psg']['values_match'] if comparison['psg'] else False,
                          'psg_write_cycles_identical':comparison['psg']['cycle_timestamps_exact'] if comparison['psg'] else False,
                          'field_differences':comparison['fields'],
                          'errors':comparison['errors']}
        if not comparison['ok']:
            failures += [f'frame {frame}: {e}' for e in comparison['errors']]
        if record['native'] != record['interpreter']:
            failures.append(f'frame {frame}: dungeon state differs between CPUs')
        record['memory_snapshot'] = compare_memory(pair_dir, frame, failures)
        result['checkpoints'].append(record)
    for mode in ('native','interpreter'):
        failures += [f'{mode}: {e}' for e in verify_progress(collected[mode])]
    result['alternate_routes'] = []
    for label, actions, frame, want_position, want_direction in ALTERNATE_ROUTES:
        pair_dir = base/label
        pair_dir.mkdir()
        record = {'route':label,'inputs':actions,'frame':frame,
                  'expected_position':want_position,
                  'expected_direction':want_direction}
        for mode in ('native','interpreter'):
            save = pair_dir/f'{mode}.sav'
            save.write_bytes(saved[mode])
            rows,state,image = invoke(exe,rom,pair_dir,mode,frame,save,route_events(slot,actions))
            record[mode]=state
            if save.read_bytes()!=saved[mode]:
                failures.append(f'{label}, {mode}: persistent SRAM changed')
            if state['game_mode']!=0x0B or state['position']!=want_position or \
               state['direction']!=want_direction or state['dungeon_index']!=2:
                failures.append(f'{label}, {mode}: dungeon route has wrong final position/facing')
            if transition_log(rows)!=list(EXPECTED_MODE_TRANSITIONS):
                failures.append(f'{label}, {mode}: wrong game-mode transition log')
            shutil.copyfile(pair_dir/f'{mode}_{frame}.csv',pair_dir/f'{mode}.csv')
            shutil.copyfile(pair_dir/f'{mode}_{frame}.psg.csv',pair_dir/f'{mode}.psg.csv')
            shutil.copyfile(image,pair_dir/f'{mode}.png')
        comparison=compare(pair_dir,frame)
        record['parity']={'ok':comparison['ok'],
                          'image_identical':comparison['images_match'],
                          'psg_values_identical':bool(comparison['psg'] and comparison['psg']['values_match']),
                          'errors':comparison['errors']}
        if not comparison['ok']:
            failures.extend(f'{label}: {e}' for e in comparison['errors'])
        if record['native']!=record['interpreter']:
            failures.append(f'{label}: native and interpreter dungeon state differs')
        record['memory_snapshot']=compare_memory(pair_dir,frame,failures)
        result['alternate_routes'].append(record)
    result['ok']=not failures
    result['errors']=failures
    result['frames_final']=CHECKPOINTS[-1][0]
    result['tiles_traversed']=7
    result['roundtrip_corridor_steps']=14
    result['south_branch_steps']=6
    result['separate_native_interpreter_save_files']=True
    result['combat_verified']=False
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--rom',required=True,type=Path)
    ap.add_argument('--out',required=True,type=Path)
    ap.add_argument('--slot',type=int,choices=(1,2),default=1)
    args=ap.parse_args()
    try: result=run(args.rom,args.out,args.slot)
    except (ValueError,FileNotFoundError,FileExistsError,subprocess.SubprocessError) as exc:
        ap.error(str(exc))
    print(json.dumps({'ok':result['ok'],'checkpoints':len(result['checkpoints']),
                      'tiles_traversed':result.get('tiles_traversed'),
                      'errors':result['errors']},indent=2))
    return 0 if result['ok'] else 1


if __name__=='__main__':raise SystemExit(main())
