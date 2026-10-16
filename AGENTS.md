# Phantasy Star — automated native recompilation contract

- This repository tracks the **game-specific, distributable source and tests only**. Never commit copyrighted ROMs, ROM-equivalent generated C, extracted commercial assets, executable binaries, memory dumps, screen captures, or audio.
- Primary target is the supplied 512 KiB English Rev. 3 SMS ROM, verified by SHA-256 before every generation or run. Japanese ROM work must use a separate game config and independent verification.
- Recreate the exact Stage 10 patched engine using `python tools/bootstrap_engine.py --engine-zip ... --core-zip ...`. It compares upstream hashes before applying all patches and refuses unexpected dependency revisions.
- Changes belong in the recompiler, runtime, tests or `patches/`, **not** in `generated/`. Regenerate generated C from the original ROM after generator changes.
- Every new gameplay claim requires a native versus interpreter replay, machine-state comparison, and a failing-then-passing regression. The bundled interpreter is not an independent emulator; confirm later against Mesen or equivalent.
- Keep unmatched state differences and sound sample differences visible. Do not conflate screenshots, register write sequences, or frame hashes with exact CPU, cycle, or PCM parity.
- Prefer integrated whole-game progress (menu, map, battle, save) over new bank-wide `.DB` inventories.
- No agent may disable or weaken the verification gates to pass a check; use explicit quarantined failures and restore exact evidence.
- The current tested checkpoint is Stage 10; inspect `docs/STAGE10_VALIDATION.md` before claiming broader coverage.

- For cartridge save tests, assert actual file-byte equality in addition to frame parity. Never report a working SAVE/CONTINUE flow based only on backup-RAM initialization.
