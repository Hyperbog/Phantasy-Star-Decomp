# Phantasy Star — Native SMS Recompilation

**Project:** *Phantasy Star*, Sega Master System, English Rev. 3, 1987. Game-specific repo for ROM-driven Z80 → C static recompilation, a mapper-aware hybrid interpreter fallback, and automated native/interpreter parity testing. **Current checkpoint: Stage 12 (SAVE/CONTINUE-backed dungeon exploration).**

**Status:** A native Linux headless runner previously compiled from the original ROM. The Stage 9 private build reached 14,500 scripted frames, including title, introduction, Alis's home/dialogue, and overhead town. Native and bundled-interpreter runs had byte-identical final RAM, VRAM, CRAM, VDP registers, and frame PNG; 52,395 PSG writes matched by value in order. **Not a verified playable alpha**: per-write PSG timing and PCM waveforms differ; some intermediate memory snapshots disagree; no independent emulator parity; battles untested and SAVE/CONTINUE validated only for slots 1 and 2. See `docs/STAGE9_VALIDATION.md`. The separate repository-bootstrap reproduction is recorded in `docs/REPOSITORY_BOOTSTRAP_VALIDATION.md`.

## Stage 10: cartridge save-memory support

The Stage 9 mapper did not implement the original cartridge SRAM window.
Stage 10 adds mapper `$FFFC` SRAM enable/bank selection, 8/16/32 KiB raw
save-file compatibility, optional `--sram-file`, and a save-image parity check.
The original game initializes backup SRAM correctly and native/interpreter
images match through the extended 14,500-frame town replay.
**Stage 11 now verifies actual SAVE and CONTINUE slot operations;** this Stage 10 paragraph documents the earlier hardware-only milestone.
See [`docs/STAGE10_VALIDATION.md`](docs/STAGE10_VALIDATION.md).

## Stage 11: in-game SAVE and CONTINUE

Stage 11 closes two native control-flow gaps: interpreter fallback return
tracking when the game deliberately unwinds extra stack frames, and generated
C falling off an SMS slot boundary (`$3FFF` to `$4002`). The fixes live in
`patches/stage11-control-flow.patch` and are applied to the source generator
and runtime; generated game code is **never** edited or committed.

Full two-process in-game SAVE/CONTINUE tests now pass for **slots 1 and 2**.
Both native execution and the bundled interpreter write identical cartridge
save files, then restart, select CONTINUE and restore 1,023 unchanged bytes of
the game's 1 KiB save-state payload. A third replay verifies that CONTINUE
against an *empty slot* does not restore gameplay. See
[`docs/STAGE11_VALIDATION.md`](docs/STAGE11_VALIDATION.md).

```bash
python3 tools/run_save_roundtrip.py --rom '/path/English Rev3.sms' --slot 1 --out runs/slot1
python3 tools/run_save_roundtrip.py --rom '/path/English Rev3.sms' --slot 2 --out runs/slot2
```

This verifies the named character save and reload for two exercised slots.
Overwrite/delete, the other three slots, combat, independent Mesen parity,
and exact PSG timestamp/audio-sample accuracy remain open.

## Stage 12: dungeon navigation after saved-game restart

The Stage 12 harness runs the original **SAVE → exit → CONTINUE** transaction,
then guides the restored character to a dungeon using controller inputs. It
validates game mode transitions `08 → 09 → 0A → 0B`, facing direction, actual
map-coordinate updates, and two-dimensional navigation. Nine checkpoints cover
seven distinct westward tile moves (`$1E → $17`). Three additional routes
verify seven eastward return moves, three southward moves, and the return north.
The routes are tested from **both save slots** in both native and bundled
interpreter modes. Each of the resulting 24 paired snapshots has byte-exact
VRAM/CRAM/VDP registers and rendered PNGs; differences in system RAM are
restricted to identified stack-area bytes. PSG write **values**, not their
exact timestamps/audio waveforms, match.

A required progress assertion prevents a pair of identically frozen game
executions from counting as a pass. Dungeon encounters and battle menus are
**not** verified by this test; the chosen entrance/corridor did not generate a
battle. See [`docs/STAGE12_VALIDATION.md`](docs/STAGE12_VALIDATION.md).

```bash
python3 tools/run_dungeon_route.py --rom '/path/Phantasy Star English Rev3.sms' \
    --slot 1 --out runs/dungeon_slot1
python3 tools/run_dungeon_route.py --rom '/path/Phantasy Star English Rev3.sms' \
    --slot 2 --out runs/dungeon_slot2
python3 -m unittest discover -s tests -v
```

The test starts from a fresh, verified in-game save and refuses to reuse an
existing evidence output directory. Do not add save images, original ROMs,
generated C, screenshots, or binaries to the public repository.

## Repository contents

- `game.toml` — English Rev. 3 mapper, game RAM layout, and four ROM-checked control-flow tables.
- `tools/bootstrap_engine.py` — fail-closed assembly of the upstream Z80 recompiler and core dependency from source ZIPs.
- `patches/stage9-runtime-and-generator.patch`, `patches/stage10-sram.patch`, and `patches/stage11-control-flow.patch` — Stage 7–10 fixes to the *upstream* toolchain, including mapper-safe dispatch, fallback return handling, tracing and replay input parsing.
- `regenerate.sh`, `build.sh`, `run.sh` — exact original-ROM guarded generation, compilation and execution.
- `tools/run_scenario.py`, `tools/run_save_roundtrip.py`, and `tools/run_dungeon_route.py` — gameplay replays, actual saves and fresh-process continuation, and input-driven dungeon traversal.
- `tests/` — source-only repository consistency checks, runnable in CI without ROM data.
- `docs/` — preserved Stage 9 accuracy results and limitations.

**Important:** No commercial ROM, ROM-derived auto-generated C, native executable, third-party engine source, or commercial game screenshots are tracked in this Git repository. The engine and game-generated code are recreated locally.

## Reproduce Stages 11–12 (Linux)

Requirements: Python 3.11+, GCC, CMake, `git` (for `git apply`), and your legitimate English Rev. 3 ROM. The exact ROM SHA-256 is `9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`.

1. Supply the original upstream `smsggrecomp-main.zip` and `z80-recomp-core-main.zip` **matching the hashes** recorded in `patches/source_archives.json`. Do not extract these inside the repository.
2. Run the following, adjusting file paths:

```bash
python3 tools/bootstrap_engine.py --engine-zip /path/smsggrecomp-main.zip --core-zip /path/z80-recomp-core-main.zip
bash regenerate.sh '/path/to/Phantasy Star English Rev3.sms'
python3 tools/run_scenario.py --rom '/path/to/Phantasy Star English Rev3.sms' --scenario city9700
python3 tools/run_scenario.py --rom '/path/to/Phantasy Star English Rev3.sms' --scenario citysouth14500
python3 -m unittest discover -s tests -v
```

`regenerate.sh` compiles the upstream C generator, emits the entire discovered native C in ignored `generated/`, and builds ignored `bin/PhantasyStarNative`. It **rejects** ROM hash changes and unverified `dispatch_manifest.txt` feedback. The two 9,700 / 14,500-frame scenarios compare all tracked game-state fields, the last rendered PNG, the final CPU clock, and ordered PSG writes. **They do not claim exact PCM parity.**

For a fresh persistent SRAM-backed replay, use:

```bash
python3 tools/run_scenario.py --rom '/path/Phantasy Star English Rev3.sms' \
  --scenario citysouth14500 --with-sram --out runs/stage10-sram
```

To run manually with persistence, add `--sram-file /path/mygame.sav`
to the native runner command. Its file is never committed to Git.

For a quick 600-frame replay, use `--scenario intro600`. The project's code-generation target is *reproducible* rather than hand-written C; do not commit generated output.

The source-only repository is published at
[Hyperbog/Phantasy-Star-Decomp](https://github.com/Hyperbog/Phantasy-Star-Decomp).
Keep ROM files, generated C, private save files, screenshots, traces and
binaries out of the public repository; `.gitignore` excludes them.

## Licensing and accuracy

The upstream `smsggrecomp` project does not declare a top-level license in the supplied snapshot. The core carries a PolyForm Noncommercial license and superzazu Z80 is MIT. This repo does not grant redistribution or commercial rights in any external sources or commercial game material. See `THIRD_PARTY.md`. **A private origin is preferable until licensing is reviewed.**
