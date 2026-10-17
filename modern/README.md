# Modern Phantasy Star — independent engine

This folder is the **modernization workspace**. The repository-root
recompiler, patches, ROM-gated tooling, tests and Stage 12 gameplay evidence
remain unchanged and form the **original-game reference**.

## Build and test

Requires CMake 3.20+ and a C++20 compiler. No ROM, asset extraction, SDL,
engine ZIP or network dependency is needed for this initial library.

```sh
cmake -S modern -B modern/build -DCMAKE_BUILD_TYPE=Debug
cmake --build modern/build --parallel 2
ctest --test-dir modern/build --output-on-failure
```

Current module: `modernps_core`, a deterministic two-combatant domain model
with explicit turn order, validation, configurable placeholder damage rules,
HP accounting and typed results. Its eight tests check invariant preservation,
turns, defeat, rules and arithmetic overflow safety.

**No combat formula has yet been validated against the Phantasy Star ROM.**
Do not promote these placeholder rules into a `classic` preset until confirmed
by ROM-specific tracing and independent emulator checks. No gameplay, renderer,
SDL integration, actual character data, map or asset has been imported.

Read [the architecture contract](docs/ARCHITECTURE.md) before adding modules.

## World, character and content foundation

The portable engine also provides:

- **WorldMap/WorldState**: finite 128x128 tile grid maximum, explicit spawn,
  wall/out-of-bounds movement results, and facing changes even when blocked.
- **Character/Party**: stat growth, cumulative experience thresholds, a level-99
  cap, and up to four distinctly identified party members.
- **Versioned content**: strict, dependency-free `PSMAP 1` and `PSCHAR 1`
  readers. The sample documents in `content/` are **original test data**;
  no assets, level tables or game rules have been extracted from the ROM.
- **Integration test**: loads real sample files, navigates a room, rejects a
  wall, uses two content-derived actors in combat, then grants XP to the party.

All of these are **illustrative modern engine mechanics**. The XP curve,
movement rules and combat formulas are NOT verified as authentic Phantasy Star
behavior. They provide clean interfaces for future reconstructed classic rules.

See [the content schema](docs/CONTENT_FORMAT.md) for mod authoring and its
validation constraints.
