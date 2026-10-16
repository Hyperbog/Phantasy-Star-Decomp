# Stage 9 — independent clean-extraction build/replay evidence

Performed after creating the private Stage 9 archive, using an empty extraction
directory and the exact English Rev. 3 user-supplied ROM externally.

- ZIP CRC integrity: `unzip -t` found no errors.
- Fresh Linux native build: `bash build.sh` **PASS**.
- Unit/integration evidence tests: `python3 -m unittest discover -s tests -v`
  **31/31 PASS**.
- Fresh CMake/GCC recompiler build plus full ROM regeneration:
  `bash regenerate.sh <ROM>` **PASS**. All four resulting `generated/ps1en_*`
  file SHA-256 hashes were identical to those bundled before regeneration.
- Replayed `citysouth14500` from the rebuilt, regenerated executable:
  **PASS**. 14,500 frames, same tracked fields, identical final PNG, matching
  final CPU cycle counts, and identical 52,395 PSG write values.
- Native-executed share for the 14,500-frame path:
  `809835370 / 866172011` Z80 cycles = **93.496%**; seven distinct
  interpreter fallback dispatch misses.

**Not passed / not asserted:** PSG write timestamps are not equal, native vs
reference PCM waveform is not byte-equal, and mid-run RAM/VRAM/cycle states
have intermittent differences. The reference is the bundled interpreter, not
an independent emulator. No original copyrighted ROM image is included.
