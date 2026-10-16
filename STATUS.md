# Current checkpoint — Stage 12: input-driven dungeon exploration

- Repository: `https://github.com/Hyperbog/Phantasy-Star-Decomp`, source-only `main`.
- Target ROM: exact English Rev. 3 512 KiB, SHA-256 `9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`.
- Prior Stage 11: original in-game SAVE/CONTINUE across separate processes in slots 1 and 2, plus negative empty-slot controls.
- Stage 12: from the restored saved game, enter dungeon modes 0A/0B, turn, traverse seven westward tiles, return east, traverse three southward tiles, and return north.
- Both SAVE slots 1 and 2 run the Stage 12 path. The 24 paired native/interpreter endpoint checks have byte-identical PNG, VRAM, CRAM, VDP registers, and ordered PSG write values; control-state fields agree. Minor differences in sampled RAM are confined to the system stack region.
- The progress judge fails if both CPUs freeze in the same place; mode/position/direction are asserted at each checkpoint. 51 source-only unit tests pass.
- **Pending and unverified:** enemy encounters, battle logic, dungeon items, other dungeons, save overwrites, PSG exact timing and PCM waveforms, intermediate full-RAM/VRAM parity, independent emulator (Mesen/hardware), Windows executable.
- Engine/generator are unchanged in Stage 12. All new code is reproducible testing and documentation; no ROM assets/generated C go in the public Git history.
- See `docs/STAGE12_VALIDATION.md` for exact evidence, scripts and limitations.
