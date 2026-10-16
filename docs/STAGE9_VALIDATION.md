# Stage 9 — extended city/map and sound validation

Target: **Phantasy Star English Rev. 3** (Sega Master System), original supplied
524,288-byte ROM, SHA-256
`9e324e92bd833b45126861b6f1d2dcbb51251bdf3df6d563ab3a3ff784d8cf88`.
No commercial ROM is in this package.

## Genuine changes since Stage 8

1. `engine/runner/main.c` replaces the silent **32-controller-event limit**
   with 512 entries. Input overflow, invalid frames/keys and unknown command-line
   switches now fail with exit code 2 instead of silently ignoring requested
   replay actions. `--check-input` validates a schedule without opening a ROM.
   No CPU/gameplay instructions were patched.
2. `engine/runner/glue.c` now records the **active execution clock** for PSG
   writes: native `g_z80.cyc`, interpreter `g_hz.cyc`, or hybrid
   `g_hybrid_base+g_hz.cyc`, instead of using the frozen native snapshot.
   This is **telemetry only**, not an audio-timing fix.
3. `tools/run_scenario.py` adds deterministic `city9700` and
   `citysouth14500` input paths (42 and 44 events) and a PSG value-sequence
   comparison. It intentionally records, but does not treat as a pass, exact
   cycle-timestamp parity.
4. `tests/test_stage9.py` adds eight regression tests for new replay evidence,
   exact final graphics, event overflow/error handling and audio disclosures.
   The previous 23 Stage 8 tests still pass.

## Actual same-runtime verification

| Replayed input path | Frames | Mode transitions | PSG writes (each run) | Video/logic state | Last PNG | Last 8 KiB RAM |
|---|---:|---:|---:|---|---|---|
| Home/dialogue and town-map navigation | 9,700 | 21 | 34,944 | 12 tracked fields all frames match | byte-identical | byte-identical |
| Extended town-map input path | 14,500 | 21 | 52,395 | 12 tracked fields all frames match | byte-identical | byte-identical |

Both paths end in mode `$09` (map exploration). Both match the interpreter's
end-of-run CPU cycle total (`579439204` / `866172011`). Full VRAM, palette,
VDP registers, scroll registers, and RAM dumps are byte-identical at both
endpoints. The two runs have exactly eight **transient VRAM hash differences**
at frames `47, 53, 857, 4143, 4153, 4457, 5057, 5857`. Many intermediate
full-RAM hashes differ (the frame boundary can sample different stack residues)
and ~97 intermediate cycle samples differ on the shorter path. This is **not**
all-frame CPU/RAM/cycle equivalence.

All 34,944 / 52,395 PSG values match in order. Absolute cycle timestamps are
**not exact** between the two modes; their first difference is at PSG write
17 (one-indexed). This does not establish exact PSG waveform timing.

To verify synthesized audio rather than equating PSG commands with samples,
Stage 9 produced native and interpreter stereo S16 WAV files through the
600-frame intro. Both were 8,960,444 bytes, but **their SHA-256 hashes and
PCM samples differ**. First sample difference is at interleaved S16 sample
453,566, and 3,992,742 of 4,480,200 interleaved samples differ. See
`evidence_stage9/PSG_WAV_600_DIFF.json`; the generated WAVs are not bundled.
This is an open audio accuracy issue, not a passing audio-equivalence test.

## Reproduction

```sh
bash build.sh
python3 -m unittest discover -s tests -v
python3 tools/run_scenario.py --rom '/path/to/EnglishRev3.sms' --scenario city9700
python3 tools/run_scenario.py --rom '/path/to/EnglishRev3.sms' --scenario citysouth14500
bin/PhantasyStarNative --check-input --press 310:A --press 315:
```

The replay script independently launches native and interpreter runs with the
same ROM and inputs and checks every tracked state field, final PNG, final
CPU clock and ordered PSG values; it returns an error on a required mismatch.
The full per-frame and per-write records, screenshot pairs, last frame dumps,
and `result.json` are bundled in `evidence_stage9/`.

## What is not established

- The reference is a **bundled Z80 interpreter sharing VDP/PSG/bus components**;
  there is no independent emulator/hardware comparison.
- PSG **waveform output is not identical**, despite identical write values.
- Game input automation has not yet demonstrated a complete battle or tested
  save/load persistence; adding those claims would be incorrect.
- The executable is still a Linux headless native recompiler with interpreter
  fallback, not a full-game native alpha or a Windows release.
- Static-generated C code and executable are derived from copyrighted game
  code, and their redistribution must be treated as private.

## Final clean-run certification

See `STAGE9_CLEAN_VALIDATION.md`: clean ZIP extraction and C build passed;
31 tests passed; four generated C files retained identical hashes after
whole-ROM regeneration; and the clean 14,500-frame replay passed. The actual
14,500-frame execution share is **93.496% native / 6.504% fallback**, with
seven distinct dispatch misses. The generator build has an unused-parameter
compiler warning; it is not reported as a warning-free build.
