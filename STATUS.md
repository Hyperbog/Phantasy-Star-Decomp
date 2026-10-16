# Current checkpoint — Stage 10: cartridge SRAM

- Public source-only repository: `https://github.com/Hyperbog/Phantasy-Star-Decomp`, branch `main`.
- Target ROM: English Rev. 3, 512 KiB, SHA-256 `9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`.
- Stage 10 adds opt-in cartridge SRAM persistence, physical mapper bank switching, verified 8/16/32 KiB file handling, hardware-level C tests, and RAM-image parity in paired gameplay replays.
- 600-, 9,700-, and 14,500-frame native and same-runtime interpreter paths with SRAM enabled passed. Both modes produced identical initialized backup-RAM images; the game backup header is present.
- Native execution still contains interpreter fallback; no generated ROM-equivalent code is tracked.
- Pending: **in-game SAVE/CONTINUE slot operations**, battle coverage, PSG per-write timing/PCM parity, transient VRAM/RAM/cycle discrepancies, independent emulator comparison, Windows release.
- Do not interpret exact end-frame parity, generated function counts, or native-cycle share as verified whole-game completion.
- Validation and source-only reproducibility: `docs/STAGE10_VALIDATION.md` and `docs/STAGE9_VALIDATION.md`.
