# Phantasy Star — Native SMS Recompilation

**Project:** *Phantasy Star*, Sega Master System, English Rev. 3, 1987. Game-specific repo for ROM-driven Z80 → C static recompilation, a mapper-aware hybrid interpreter fallback, and automated native/interpreter parity testing. **Current checkpoint: Stage 10 (cartridge SRAM).**

**Status:** A native Linux headless runner previously compiled from the original ROM. The Stage 9 private build reached 14,500 scripted frames, including title, introduction, Alis's home/dialogue, and overhead town. Native and bundled-interpreter runs had byte-identical final RAM, VRAM, CRAM, VDP registers, and frame PNG; 52,395 PSG writes matched by value in order. **Not a verified playable alpha**: per-write PSG timing and PCM waveforms differ; some intermediate memory snapshots disagree; no independent emulator parity; battles and saves untested. See `docs/STAGE9_VALIDATION.md`. The separate repository-bootstrap reproduction is recorded in `docs/REPOSITORY_BOOTSTRAP_VALIDATION.md`.

## Stage 10: cartridge save-memory support

The Stage 9 mapper did not implement the original cartridge SRAM window.
Stage 10 adds mapper `$FFFC` SRAM enable/bank selection, 8/16/32 KiB raw
save-file compatibility, optional `--sram-file`, and a save-image parity check.
The original game initializes backup SRAM correctly and native/interpreter
images match through the extended 14,500-frame town replay.
**An actual saved-game slot and CONTINUE restoration are not yet validated.**
See [`docs/STAGE10_VALIDATION.md`](docs/STAGE10_VALIDATION.md).

## Repository contents

- `game.toml` — English Rev. 3 mapper, game RAM layout, and four ROM-checked control-flow tables.
- `tools/bootstrap_engine.py` — fail-closed assembly of the upstream Z80 recompiler and core dependency from source ZIPs.
- `patches/stage9-runtime-and-generator.patch` and `patches/stage10-sram.patch` — Stage 7–10 fixes to the *upstream* toolchain, including mapper-safe dispatch, fallback return handling, tracing and replay input parsing.
- `regenerate.sh`, `build.sh`, `run.sh` — exact original-ROM guarded generation, compilation and execution.
- `tools/run_scenario.py` — original ROM versus same-runtime interpreter paired gameplay replays.
- `tests/` — source-only repository consistency checks, runnable in CI without ROM data.
- `docs/` — preserved Stage 9 accuracy results and limitations.

**Important:** No commercial ROM, ROM-derived auto-generated C, native executable, third-party engine source, or commercial game screenshots are tracked in this Git repository. The engine and game-generated code are recreated locally.

## Reproduce Stage 9 (Linux)

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

## GitHub remote (private recommended)

This archive is an initialized Git repository, with commits on `main`. If GitHub creation is not available from the current connector, create an empty **private** repository named `Phantasy-Star-Decomp` on GitHub (do not initialize it with README or license) and run:

```bash
git remote add origin https://github.com/YOUR-ACCOUNT/Phantasy-Star-Decomp.git
git push -u origin main
```

The whole history can also be reconstructed from the separately distributed `.bundle` file via `git clone Phantasy-Star-Decomp.bundle Phantasy-Star-Decomp`.

## Licensing and accuracy

The upstream `smsggrecomp` project does not declare a top-level license in the supplied snapshot. The core carries a PolyForm Noncommercial license and superzazu Z80 is MIT. This repo does not grant redistribution or commercial rights in any external sources or commercial game material. See `THIRD_PARTY.md`. **A private origin is preferable until licensing is reviewed.**
