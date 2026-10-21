# Stage 13 — encounter reconnaissance (unverified)

The Stage 12 checkpoint remains the last ROM-validated gameplay milestone.
This tool **does not** claim a battle, a playable alpha, or additional game
progress. The English Rev. 3 ROM and matching upstream archives are needed to
run it; none are distributed by this repository.

`tools/scan_encounters.py` replays Stage 11's actual in-game SAVE and
fresh-process CONTINUE in either verified slot 1 or 2, then replays independent
controller-only branches through the known Stage 12 dungeon entrance. Default
paths include the verified west corridor and return plus exploratory west and
south extensions. They are **hypotheses**, not established reachable areas.

Each route starts from its own copy of the real saved cartridge and runs both
native and bundled-interpreter CPUs. A **mandatory Stage 12 positive control**
runs even if custom routes omit it: the known left-then-seven-west route must
produce dungeon position $1E at frame 3120 and $17 at frame 4030, with
mode $0B, facing 3, dungeon index 2 and matching native/interpreter results.
Both checkpoints use new processes and independent authentic save-file copies.
The early checkpoint receives its own parity, VRAM/CRAM/VDP/RAM and SRAM
integrity checks. A pair of identically stalled runs fails the positive
control, regardless of whether their native/interpreter screenshots match.

The scanner also rejects failed SAVE/CONTINUE prerequisites, route names or
inputs outside the known grammar, incorrect or nonchronological mode histories,
changed SRAM, mismatched compared machine-state fields, rendered images, PSG
write values, video RAM, or non-stack work RAM. Known stack drift and PSG
timing differences remain reported, not hidden. A successful reconnaissance
run establishes comparator agreement plus the explicitly checked Stage 12
movement; it does **not** establish that exploratory paths moved or that
an enemy encounter occurred.

Extra game modes are classified only as **unclassified mode transitions**;
no battle is inferred solely from a mode value, enemy RAM byte, or matching
screenshots. A future battle milestone requires a known route, verified battle
entry and commands, HP/EXP changes, enemy turns, completion, and independent
emulator confirmation.

After provisioning the exact original ROM and rebuilding with the documented
Stage 11 hashes:

```sh
python3 -m unittest discover -s tests -v
python3 tools/scan_encounters.py --rom '/path/English Rev3.sms' \
  --slot 1 --out runs/stage13_slot1
python3 tools/scan_encounters.py --rom '/path/English Rev3.sms' \
  --slot 2 --out runs/stage13_slot2
```

Custom inputs use `--route name=UDLR`, repeatable up to 16 paths; each is
limited to 32 directional presses as in Stage 12. The control may add one
internally reserved path when no supplied path uses the exact known corridor
inputs. The `stage12_positive_control` field in `result.json` records the
verified/mismatch verdict. Every evidence output folder must be new. Save
files, RAM dumps, frame PNGs and generated C remain private.
