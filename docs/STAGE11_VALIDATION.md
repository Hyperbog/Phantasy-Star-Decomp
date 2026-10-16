# Stage 11 — In-game SAVE/CONTINUE and native control-flow correction

Target: Phantasy Star (SMS, English Rev. 3), ROM SHA-256
`9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`.
This document describes **tested gameplay paths**, not a claim that the whole
RPG or all cartridge save cases are accurate. The bundled superzazu interpreter
shares the runtime hardware model with the native build: **Mesen/hardware
validation remains pending**.

## What was fixed

1. The ROM's SAVE handler performs a deliberate `POP HL; POP HL; RET` to unwind
   more than one call frame before entering name input. The hybrid interpreter's
   return detector formerly required `sp0 == entry_sp`, so a `RET` at a stack
   pointer *above* the entry slot escaped its detector and interpreted past the
   caller boundary. Stage 11 accepts a RET when the real Z80 stack has reached
   **or exceeded** that boundary using a signed 16-bit displacement.
2. The name-entry input handler ends slot 0 with the 3-byte Z80 instruction
   starting at `$3FFF`, then must execute the instruction at `$4002` in slot 1.
   Static code discovery intentionally stops at the slot boundary to avoid
   inventing mapper state, but code generation previously emitted no tail
   transfer. The generic emitter now routes any executable cross-slot
   *fall-through* through the **live bank-aware dispatcher**. On the observed
   path, `$4002` uses the interpreter fallback. This is a generator fix, not a
   hand modification to ROM-derived `generated/` source.

The two bugs had different signatures: prior to the first fix, native gameplay
never left name-entry **loading mode `$10`**; after fixing fallback unwinding it
reached **mode `$11`**, but cursor navigation/END selection failed until the
cross-slot tail was restored. Both defects are protected by source-level
regression gates and actual ROM-driven replay comparisons.

## Verified scenarios

`tools/run_save_roundtrip.py` performs three independent **process launches**
per execution mode (native and interpreter), independently for slots 1 and 2.
It refuses an existing evidence folder, and verifies the exact ROM hash first.

* **SAVE**: From the scripted opening, navigate to the command menu, select
  SAVE, accept numbered slot 1 or 2, confirm YES, enter character name `A`,
  navigate to END, and return to gameplay. 9,150 frames per mode.
* **CONTINUE**: Restart a new process using that mode's *same persisted SRAM*
  and choose CONTINUE, YES, the matching slot. Verify restored map mode at
  frame 1,760 and final frame 2,300. The original save bytes must remain
  unchanged across the restart.
* **Negative control**: Launch a third new process using erased cartridge RAM
  and the exact same CONTINUE inputs. The game initializes backup memory but
  the **empty slot must not resume gameplay** (final mode `$03`, and selected
  slot flag stays zero).

All three phases pass the existing native/interpreter 12-field frame comparison,
identical final PNG checks, final effective clock, and ordered PSG register-write
value checks. The 32 KiB SRAM save image is byte-identical between native and
interpreter for the same slot; slot occupancy and its 1 KiB saved gameplay
payload are asserted. During resume, the remaining `$C301–$C6FF` 1,023 bytes
match the saved SRAM payload exactly; the first byte `$C300` is changed during
map setup. The test asserts this explicitly rather than pretending the whole
8 KiB RAM was identical.

### Measured differences (retained, not waived)

| Sample | Save 9,150 frames | Continue 2,300 frames |
|---|---:|---:|
| Native/interpreter final VRAM bytes different | 0 | 0 |
| Native/interpreter final CRAM / VDP register bytes different | 0 | 0 |
| Final work-RAM bytes different | 1 (stack) | 2 (stack) |
| Transient frame VRAM hash differences (slot 1) | 13 | 6 |
| Ordered PSG writes (slot 1) | 32,932, all values match | 10,535, all values match |
| PSG per-write cycle timestamps | **Not equal** | **Not equal** |

Slot 2 passes the same three phases. Its observed transient VRAM hash
mismatch counts are 12 on SAVE and 2 on CONTINUE. Frame-by-frame full-RAM
hashes and some intermediate instruction-boundary cycles also differ; exact
PCM/WAV parity is not established. Tests intentionally report these differences
rather than passing them as exact CPU/audio parity.

The previous 600-, 9,700-, and 14,500-frame scripted town routes must continue
to pass. No original ROM bytes, generated game C, game screenshots, binaries,
SRAM files, or private trace dumps are committed to the public repository.

## Run

```sh
python3 tools/bootstrap_engine.py --engine-zip /path/smsggrecomp-main.zip --core-zip /path/z80-recomp-core-main.zip
bash regenerate.sh '/path/Phantasy Star English Rev 3.sms'
python3 -m unittest discover -s tests -v
python3 tools/run_save_roundtrip.py --rom '/path/Phantasy Star English Rev 3.sms' --slot 1 --out runs/slot1
python3 tools/run_save_roundtrip.py --rom '/path/Phantasy Star English Rev 3.sms' --slot 2 --out runs/slot2
```

The private evidence folders contain ROM-derived screenshots and save-state
bytes; they are excluded by `.gitignore`. The generator-derived C is likewise
regenerated locally and never committed.

## Not yet proven

Battle resolution, multi-character saves, overwrite/delete confirmations,
corrupt *in-game* save-slot recovery, all five slot selections, independent
Mesen or SMS hardware parity, audio sample correctness, Windows binaries, and
end-to-end RPG completion remain open. The Stage 10 SRAM bank/format caveats
still apply.
