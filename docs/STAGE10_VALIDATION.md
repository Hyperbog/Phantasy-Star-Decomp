# Stage 10 — Sega cartridge SRAM and persistent backup RAM

Date: 2026-10-16. Target: *Phantasy Star* SMS English Rev. 3, original
524,288-byte ROM, SHA-256
`9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`.
**No commercial game bytes, snapshots, native generated C, or binaries are
committed to this public repository.**

## New functionality

The Stage 9 runtime had no cartridge SRAM: reads in CPU `$8000–$BFFF` always
returned the selected ROM bank; writes in this window were ignored. The game
needs cartridge RAM to initialize its backup header and save-slot storage.
The Stage 10 patch adds:

- Sega mapper `$FFFC` bit 3: map cartridge SRAM into `$8000–$BFFF` rather
  than slot 2 ROM; disabling it restores the selected ROM page.
- `$FFFC` bit 2: select either 16 KiB SRAM bank when the cartridge has 32 KiB.
  Existing 8 KiB and 16 KiB save images are mirrored to their physical size.
- `$FFFC` bit 4: optional, uncommon system-memory SRAM overlay. The game
  exercises only the standard `$8000–$BFFF` backup-RAM window in the tested paths;
  bit-4 overlay is **not validated against independent hardware**.
- Bank-aware dispatch labels cartridge SRAM as **not a ROM code bank**;
  the fixed `$0000–$03FF` ROM region remains physical bank 0.
- Explicit `--sram-file /path/file.sav` loads an existing raw 8/16/32 KiB save
  or initializes a new 32 KiB erased (`0xFF`) image. It writes the image
  on clean exit **only when cartridge RAM was modified**. A write uses a
  temporary file and rename to avoid truncating the previous save on write
  failure. This path is opt-in; without it RAM is volatile.
- `tools/run_scenario.py --with-sram` performs fresh and independent native
  versus bundled-interpreter runs with cartridge SRAM enabled, and treats
  unequal/missing save-file images or missing backup headers as failures.
  To prevent accidental overwrite, it rejects preexisting scenario .sav files.

Hardware information: [SMS Power mapper documentation](https://www.smspower.org/Development/Mappers).
The Stage 9 baseline, including its two caveats concerning RAM/cycle samples
and PCM timing, is retained in `STAGE9_VALIDATION.md`.

## Tests actually run

1. Compiled C hardware probe (with a synthetic four-bank 64 KiB ROM, **not**
   Phantasy Star bytes): verified ROM write protection, SRAM visibility,
   separate 16 KiB banks, round-trip switch to ROM, fixed first 1 KiB,
   mirrored mapper controls, `sms_slot_bank` safety and file persistence.
2. Reloaded the 32 KiB save image into a new initialized bus instance and
   observed identical values in both cartridge SRAM banks.
3. Loaded and mutated an 8 KiB image, verifying wrap/mirroring and exactly
   8 KiB preserved on disk. Rejected a corrupt four-byte save without altering
   the file; the production CLI also rejected a malformed save.
4. Fresh English Rev. 3 **600-, 9,700-, 14,500-frame** native/interpreter
   scripted replays with save files enabled passed the Stage 9 gameplay
   comparisons. Both modes wrote **byte-identical 32 KiB SRAM files**.
   SRAM at frame 600 already begins with `PHANTASY STAR`, and contains
   **8,188 non-0xFF bytes** (game's backup-RAM initialization).
5. A separate native reboot with the 600-frame saved image retained the
   exact saved-file SHA-256; the game did not erase its initialized backup
   RAM on the second boot.
6. The **14,500-frame** run still matches 12 tracked gameplay/video fields
   at every frame, final PNG, final RAM/VRAM/CRAM/register dumps, and
   **52,395 PSG-write values** in order. As in Stage 9, eight transient
   VRAM hashes and PSG write timestamps do not all match.

### Source regeneration and clean reproduction

`tools/bootstrap_engine.py` first checks exact user-supplied upstream ZIP hashes,
then Stage 9 upstream and patched file hashes, then Stage 10 **parent hashes**,
applies `patches/stage10-sram.patch` and verifies the new resulting checksums.
This is source-only and repeatable without internet access.

```sh
python3 tools/bootstrap_engine.py \
  --engine-zip /path/smsggrecomp-main.zip \
  --core-zip /path/z80-recomp-core-main.zip
bash regenerate.sh '/path/Phantasy Star (EU,US) (REV 3).sms'
python3 -m unittest discover -s tests -v
python3 tools/run_scenario.py --rom '/path/EnglishRev3.sms' \
  --scenario citysouth14500 --with-sram --out runs/stage10-fresh
```

`--with-sram` requires a fresh output directory for each run. The resulting
`.sav` files, traces, game C, commercial ROM, PNG images, compiled binaries
and engine source stay outside Git history.

## Deliberately not claimed

- **In-game SAVE/CONTINUE slot semantics remain unverified.** The game initializes
  backup SRAM and a real restart preserves it, but the attempted menu input did
  not demonstrate a newly written save slot or a restored character.
- No independent Mesen, hardware, PAL, or Japanese-ROM SRAM test.
- This implementation does not guarantee arbitrary Sega mapper edge cases
  such as unusual ROM-write, bank-shift, or system-RAM overlay behavior.
- Audio values, not their write timestamps or rendered PCM samples, match.
- Battle execution is not covered. Stage 9's native fallbacks and transient
  RAM/VRAM differences remain open.
