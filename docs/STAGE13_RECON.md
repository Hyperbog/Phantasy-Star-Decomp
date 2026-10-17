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
native and bundled-interpreter CPUs. The scanner rejects failed SAVE/CONTINUE
prerequisites, route names or inputs outside the known grammar, incorrect mode
prefixes, changed SRAM, mismatched mode histories, compared machine-state
fields, rendered images, PSG write values, video RAM, or non-stack work RAM.
Known stack drift and PSG timing differences remain reported, not hidden.

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
limited to 32 directional presses as in Stage 12. Every evidence output folder
must be new. Save files, RAM dumps, frame PNGs and generated C remain private.
