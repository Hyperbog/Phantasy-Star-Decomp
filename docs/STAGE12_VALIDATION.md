# Stage 12 validation — native dungeon traversal and progress gates

**Status:** 2/2 independently saved slots and 24/24 native/interpreter paired dungeon endpoints passed. Existing ROM-free suite: **51/51 passed**.

## Goal and baseline

Continue Stage 11's functioning Linux native Z80-to-C build, cartridge SRAM, and SAVE/CONTINUE workflow into a genuine dungeon route, using only input replay (no RAM patches, teleportation, synthetic combat triggers, or edited game code). Reuse the exact original English Rev. 3 ROM (512 KiB; SHA-256 `9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`).

For each save slot (1 and 2), `tools/run_dungeon_route.py` performs Stage 11's actual in-game SAVE and separate-process CONTINUE in native and interpreter modes, including the prior empty-slot negative control. It copies each mode's real 32 KiB SRAM image to a new file for every dungeon checkpoint. Thus each checkpoint independently boots the game with the real saved character and replays the same input history to its target frame. It refuses existing output directories and incorrect ROM identities.

## Route and observed positions

- Start menu to saved game: mode `00 → 02 → 03 → 08 → 09`. Loading the dungeon occurs at **frame 2399**, mode `0A`, with active mode `0B` from **frame 2434**.
- At frame 3000, press Left for 8 frames (direction changes from 0 to **3**). Press Up for eight frames, at 130-frame intervals, seven times. Checkpoints in frames **3120, 3250, 3380, 3510, 3640, 3770, 3900, 4030, 4300** must have positions `$1E, $1D, $1C, $1B, $1A, $19, $18, $17, $17` respectively and facing direction 3. This explicitly rejects matching but stationary executions.
- **West return:** after arriving at `$17`, two Right turns and seven Up presses face east (direction 1) and return to `$1E` at frame **5470**.
- **South branch:** three Down presses from `$1E` produce `$4E` at frame **3650**. Three Up presses restore `$1E` at frame **4040**.

For all 12 endpoint checkpoints per slot, require the exact game mode `0B`, dungeon index `2`, expected facing and position, and the observed ordered game-mode transition log. The original SRAM image must remain unchanged throughout each traversal. No encounter occurs in these deterministic routes; combat is not covered.

## Native vs bundled-interpreter evidence

| Check | Slot 1 | Slot 2 |
|---|---|---|
| Full Stage 11 SAVE/CONTINUE prerequisite | PASS | PASS |
| Dungeon corridor, nine checkpoints | 9/9 PASS | 9/9 PASS |
| West return; south advance; south return | 3/3 PASS | 3/3 PASS |
| All endpoint rendered PNG bytes | 12/12 identical | 12/12 identical |
| End-point raw VRAM / CRAM / VDP registers | 12/12 identical | 12/12 identical |
| PSG register-write values (in order) | 12/12 identical | 12/12 identical |
| Mode, waiting, timer, animation parity | 12/12 match | 12/12 match |
| Persistent cartridge SRAM changed by walking | Never | Never |
| Complete 8 KiB system RAM | **Not identical** (stack bytes only) | **Not identical** (stack bytes only) |

At slot 1, the frame 3120 RAM snapshots differ at offsets `$0ACC–$0AD1` and `$0AFA` (system addresses `$CACC–$CAD1`, `$CAFA`); later corridor snapshots differ at `$0AD0`, `$0AD1`, `$0AFA`. In both slots, all observed differing bytes are in the `$CA00–$CAFF` stack region. The judge fails if any unexpected game-state RAM byte outside that region differs. Intermediate whole-VRAM hashes can differ on earlier frames while all tested endpoint VRAM dumps match; do **not** conflate sampled endpoint video parity with cycle-exact video execution.

PSG values match, but timestamps may differ and waveform/PCM parity is **not** verified. The interpreter uses the same bus, SRAM mapper, VDP, and PSG implementation as native; this is **not independent hardware or Mesen verification**.

## Reproduction

```bash
# From the Stage 12 public-safe source tree after bootstrapping the
# exact upstream archives and regenerating C from your own original ROM:
python3 -m unittest discover -s tests -v
python3 tools/run_dungeon_route.py --rom '/absolute/path/Phantasy Star English Rev3.sms' --slot 1 --out runs/stage12_slot1
python3 tools/run_dungeon_route.py --rom '/absolute/path/Phantasy Star English Rev3.sms' --slot 2 --out runs/stage12_slot2
```

Outputs include `result.json`, per-frame trace CSVs, PSG write traces, both PNGs, and raw VRAM/CRAM/VDP register/RAM snapshots. All are **private ROM-derived evidence** and should remain uncommitted. Regenerate the runner via `bash regenerate.sh <ROM>` after supplying the exact upstream engine/core archives through `tools/bootstrap_engine.py`.

## Fail-closed controls

The Stage 12 ROM-free tests assert that a stationary trace, wrong direction, wrong dungeon index, wrong mode, missing checkpoint, wrong position, malformed route, incorrect ROM, or reused output directory fails. Mocked video/RAM endpoint files are checked for unexpected byte changes; the exception for documented stack drift never applies to general work RAM. The harness rejects mismatched SRAM content or any unexpectedly modified persistent save image. Scripts never patch the game ROM or generator output.

## Outstanding

A full enemy battle is **not** reached or verified. Future work should target a known hostile overworld area or a dungeon with encounters and test battle init, attack/magic/item/run, enemy turns, HP/EXP, victory/defeat, and return to the map. Additional work: Mesen accuracy, exact PSG cycle timestamps/PCM synthesis, remaining static-code fallbacks, save overwrites, alternate save slots, other game regions, Windows executable. Static C coverage or identical end frames must not be presented as game completion.
