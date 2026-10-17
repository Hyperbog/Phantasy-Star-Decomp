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

## Inventory, equipment, and event/quest foundation

`modernps_core` now includes `ItemCatalog`, `Inventory`, `Loadout`,
`EventEngine`, `QuestJournal` and `WorldInteractions`. Inventories enforce
per-item stack counts and a distinct-item slot limit. Equipment bonuses are
read by the combat-domain model. Consumables restore lost HP. A story action
may set a flag, grant an item, or grant XP; an invalid later action cannot
partially commit an earlier reward. World events require player interaction
on a specifically bound walkable tile. Quests are projections of story flags.

Additional strict `PSITEM 1`, `PSEVENT 1`, and `PSQUEST 1` content files
include a complete **original training** sample. The sample integration test
covers content -> map -> event -> inventory -> equipment -> combat -> healing
-> quest completion -> character progression. This is a testable logic
foundation, not a GUI, a runnable RPG, or an authentic original-game ruleset.

See [the content schema](docs/CONTENT_FORMAT.md) for supported authoring fields
and [the architecture contract](docs/ARCHITECTURE.md) for scope boundaries.

## Stage 4: dialogue, shops, economy and persistence

The `modernps_core` library now also provides:

- `DialogueEngine`: prioritized, condition-dependent NPC lines selected from
  quest/event flags, with optional tile bindings. Dialogue is text-only; no
  UI renderer or cut-scene scripting is included.
- `Shop` and `Wallet`: catalog-checked offers, buy/sell prices, capped money,
  quantity checks and strong transaction safety when trades fail.
- `GameState` with `PSAVE 1` persistence: ROM-free saves for map position and
  facing, party XP/HP, inventory, equipment, wallet and story flags/completions.
  Read restores into a **new** state; rejected saves cannot corrupt the active
  session. Disk writing uses a same-directory temporary file and rename.
- `PSNPC 1` and `PSSHOP 1`: strictly versioned standalone content definitions
  for demonstration NPC dialogue and merchant offers.

The new `modernps_campaign` integration test loads sample content and runs
shopping -> map -> loot -> equipment -> combat -> healing -> quest -> NPC
conversation -> serialization -> restored session -> trade.

Read [persistence constraints](docs/PERSISTENCE.md) before relying on save
compatibility or integrating a graphical front-end.

These are **new illustrative modern-game rules**. Nothing in this directory
claims authentic SMS item prices, NPC text or original save byte format.
