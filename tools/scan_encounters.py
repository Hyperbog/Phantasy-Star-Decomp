#!/usr/bin/env python3
"""Stage 13 reconnaissance: input-only routes after authentic SAVE/CONTINUE.

A new mode is an *unclassified observation*, not proof of combat. This tool
compares native and bundled-interpreter results, not independent hardware.
"""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path

from run_scenario import ROOT, SHA256, compare
from run_save_roundtrip import run_roundtrip
from run_dungeon_route import (
    EXPECTED_MODE_TRANSITIONS, compare_memory, invoke,
    route_events, transition_log,
)

DEFAULT_ROUTES = (
    'west_corridor=LUUUUUUU',
    'west_return=LUUUUUUURRUUUUUUU',
    'west_extension=LUUUUUUUUUUU',
    'south_extension=DDDDDDDD',
)
ROUTE_NAME = re.compile(r'[a-z][a-z0-9_-]{0,39}\Z')
CONTROL_ACTIONS = 'LUUUUUUU'  # Stage 12: turn left, walk seven westward tiles
CONTROL_ENTRY_FRAME = 3120


def parse_routes(specs):
    """Require explicit, independently named controller sequences."""
    routes = []
    seen = set()
    for spec in specs:
        name, separator, actions = spec.partition('=')
        if not separator or not ROUTE_NAME.fullmatch(name) or name in seen:
            raise ValueError(f'invalid or repeated route name: {spec!r}')
        # Use the exact Stage 12 input grammar and timing, not a new parser.
        route_events(1, actions)
        seen.add(name)
        routes.append((name, actions))
    if not routes or len(routes) > 16:
        raise ValueError('require 1..16 named routes')
    return routes


def endpoint_frame(actions):
    """Sample 120 frames after the final eight-frame directional press."""
    return 3000 + 130 * (len(actions) - 1) + 120


def classify_transitions(transitions):
    """Reject invalid histories; never infer combat from a mode change."""
    baseline = list(EXPECTED_MODE_TRANSITIONS)
    if not isinstance(transitions, (list, tuple)) or transitions[:len(baseline)] != baseline:
        raise ValueError('the saved-game/dungeon transition prefix changed')
    for index, entry in enumerate(transitions):
        if not isinstance(entry, (tuple, list)) or len(entry) != 2:
            raise ValueError('malformed transition record')
        frame, mode = entry
        if (type(frame) is not int or frame < 1 or not isinstance(mode, str)
                or not re.fullmatch(r'[0-9A-F]{2}', mode)):
            raise ValueError('invalid transition frame or mode')
        if index and (frame <= transitions[index-1][0]
                      or mode == transitions[index-1][1]):
            raise ValueError('nonchronological or duplicate transition')
    extra = transitions[len(baseline):]
    return {'additional_modes':extra,
            'unclassified_mode_change':bool(extra),
            'battle_verified':False}


def verify_corridor_progress(entry, exit_state):
    """Two real fresh-process snapshots must demonstrate Stage 12 movement."""
    errors = []
    for label, state, position in (
            ('entry', entry, 0x1E), ('exit', exit_state, 0x17)):
        for key, expected in (
                ('game_mode', 0x0B), ('direction', 3),
                ('position', position), ('dungeon_index', 2)):
            actual = state.get(key)
            if actual != expected:
                errors.append(
                    f'{label}: {key} is {actual!r}, expected {expected:02X}')
    if entry.get('position') == exit_state.get('position'):
        errors.append('Stage 12 positive control did not change dungeon position')
    return errors


def copy_pair_inputs(folder, mode, frame, image):
    for source, destination in (
        (folder / f'{mode}_{frame}.csv', folder / f'{mode}.csv'),
        (folder / f'{mode}_{frame}.psg.csv', folder / f'{mode}.psg.csv'),
        (image, folder / f'{mode}.png'),
    ):
        shutil.copyfile(source, destination)


def run(rom, out, slot=1, route_specs=DEFAULT_ROUTES, exe=None):
    rom, out = Path(rom).resolve(), Path(out).resolve()
    if out.exists():
        raise FileExistsError(f'will not reuse prior evidence: {out}')
    if not rom.is_file() or hashlib.sha256(rom.read_bytes()).hexdigest() != SHA256:
        raise ValueError('exact English Rev 3 ROM SHA-256 required')
    if slot not in (1, 2):
        raise ValueError('only previously verified SAVE slots 1 and 2')
    routes = parse_routes(route_specs)
    # Never allow custom routes to bypass the known-good movement control.
    if not any(actions == CONTROL_ACTIONS for _, actions in routes):
        routes.insert(0, ('__stage12_control', CONTROL_ACTIONS))
    exe = Path(exe).resolve() if exe else ROOT / 'bin/PhantasyStarNative'
    if not exe.is_file():
        raise FileNotFoundError(f'compiled executable absent: {exe}')
    out.mkdir(parents=True)
    setup = run_roundtrip(rom, out / 'save_restore_prerequisite', slot, exe=exe)
    report = {'ok':False, 'slot':slot, 'rom_sha256':SHA256,
              'oracle':'shared-runtime interpreter, not independent emulator',
              'battle_verified':False, 'routes':[], 'errors':[],
              'stage12_positive_control':{'verified':False}}
    errors = report['errors']
    if not setup['ok']:
        errors.extend(['SAVE/CONTINUE prerequisite failed', *setup['errors']])
    saved = {}
    if not errors:
        for mode in ('native', 'interpreter'):
            saved[mode] = (out / 'save_restore_prerequisite' / mode /
                           'cartridge.sav').read_bytes()
            if len(saved[mode]) != 0x8000 or saved[mode][0x200+slot] != 1:
                errors.append(f'{mode}: missing real in-game save in slot {slot}')
        if saved['native'] != saved['interpreter']:
            errors.append('saved cartridge images differ between CPUs')
    if errors:
        (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
        return report

    # Re-run the early corridor entry in both CPU modes from original save
    # files. An equal but motionless pair cannot certify Stage 13 progress.
    entry_folder = out / '__stage12_entry'
    entry_folder.mkdir()
    entry_states, entry_errors = {}, []
    for mode in ('native', 'interpreter'):
        save = entry_folder / f'{mode}.sav'
        save.write_bytes(saved[mode])
        rows, state, image = invoke(
            exe, rom, entry_folder, mode, CONTROL_ENTRY_FRAME, save,
            route_events(slot, CONTROL_ACTIONS))
        entry_states[mode] = state
        if save.read_bytes() != saved[mode]:
            entry_errors.append(f'{mode}: entry control modified persistent SRAM')
        if transition_log(rows) != list(EXPECTED_MODE_TRANSITIONS):
            entry_errors.append(f'{mode}: entry control mode history changed')
        copy_pair_inputs(entry_folder, mode, CONTROL_ENTRY_FRAME, image)
    entry_comparison = compare(entry_folder, CONTROL_ENTRY_FRAME)
    entry_errors.extend(f'entry control: {e}' for e in entry_comparison['errors'])
    if entry_states['native'] != entry_states['interpreter']:
        entry_errors.append('entry control: CPU gameplay states differ')
    entry_memory = compare_memory(entry_folder, CONTROL_ENTRY_FRAME, entry_errors)
    report['stage12_positive_control'] = {
        'verified': False, 'entry_frame': CONTROL_ENTRY_FRAME,
        'entry_states': entry_states, 'entry_parity_ok': entry_comparison['ok'],
        'entry_memory': entry_memory}
    errors.extend(entry_errors)

    for name, actions in routes:
        frame = endpoint_frame(actions)
        folder = out / name
        folder.mkdir()
        record = {'name':name, 'actions':actions, 'frame':frame,
                  'observations':{}, 'parity':{}, 'memory':{}}
        logs = {}
        for mode in ('native', 'interpreter'):
            save = folder / f'{mode}.sav'
            save.write_bytes(saved[mode])
            rows, state, image = invoke(
                exe, rom, folder, mode, frame, save, route_events(slot, actions))
            if save.read_bytes() != saved[mode]:
                errors.append(f'{name}, {mode}: walking modified persistent SRAM')
            logs[mode] = transition_log(rows)
            try:
                observation = classify_transitions(logs[mode])
            except ValueError as exc:
                errors.append(f'{name}, {mode}: {exc}')
                observation = {'additional_modes':[], 'unclassified_mode_change':False,
                               'battle_verified':False}
            record['observations'][mode] = {'terminal_raw_ram_fields':state,
                                             'mode_transitions':logs[mode],
                                             **observation}
            copy_pair_inputs(folder, mode, frame, image)
        if logs['native'] != logs['interpreter']:
            errors.append(f'{name}: transition histories differ')
        if (record['observations']['native']['terminal_raw_ram_fields'] !=
                record['observations']['interpreter']['terminal_raw_ram_fields']):
            errors.append(f'{name}: terminal gameplay state differs')
        comparison = compare(folder, frame)
        record['parity'] = {
            'ok':comparison['ok'], 'errors':comparison['errors'],
            'image_identical':comparison['images_match'],
            'psg_values_identical':bool(comparison['psg'] and comparison['psg']['values_match']),
            'psg_timestamps_identical':bool(comparison['psg'] and comparison['psg']['cycle_timestamps_exact']),
        }
        errors.extend(f'{name}: {error}' for error in comparison['errors'])
        record['memory'] = compare_memory(folder, frame, errors)
        report['routes'].append(record)
    control = next(record for record in report['routes']
                   if record['actions'] == CONTROL_ACTIONS)
    control_failures = list(entry_errors)
    for mode in ('native', 'interpreter'):
        terminal = control['observations'][mode]['terminal_raw_ram_fields']
        control_failures.extend(
            f'{mode}: {e}' for e in
            verify_corridor_progress(entry_states[mode], terminal))
    errors.extend(e for e in control_failures if e not in entry_errors)
    report['stage12_positive_control'].update({
        'exit_frame': control['frame'], 'exit_route': control['name'],
        'verified': not control_failures and control['parity']['ok'],
        'errors': control_failures})
    report['ok'] = not errors
    (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--slot', type=int, choices=(1, 2), default=1)
    parser.add_argument('--route', action='append', dest='routes',
                        metavar='NAME=UDLR', help='repeat for custom paths')
    args = parser.parse_args()
    try:
        result = run(args.rom, args.out, args.slot,
                     args.routes if args.routes is not None else DEFAULT_ROUTES)
    except (ValueError, FileNotFoundError, FileExistsError,
            subprocess.SubprocessError) as exc:
        parser.error(str(exc))
    print(json.dumps({'ok':result['ok'], 'routes':len(result['routes']),
                      'battle_verified':False, 'errors':result['errors']}, indent=2))
    return 0 if result['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
