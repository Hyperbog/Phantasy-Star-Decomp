# Modern engine architecture — foundation checkpoint

## Repository boundary

- **Repository root:** ROM-gated Z80-to-C recompiler, Sega Master System
  runtime and regression evidence (original-game reference). Do not move it.
- **`modern/`:** portable, platform-independent modern game logic, future
  rendering/audio adapters, content tools, and their own CMake/CI tests.
- **Shared comparison:** future adapters consume *verified behavioral traces*,
  not internal emulator RAM addresses, to test `classic` implementations.

## Current modules

`include/modernps/combat.hpp` defines engine-owned `Stats`, `Actor`,
`DamageRules`, `AttackEvent`, and `Battle`. `src/combat.cpp` implements the
rules without display, audio, file I/O, global state or randomness. A battle
starts with two living, distinct actors; the player takes the first turn.
Invalid actions raise exceptions without changing the battle state.

The current rule is an **illustrative, non-original** deterministic expression:

`damage = min(target_current_hp, max(minimum_damage,
 attacker_attack * attack_multiplier - defender_defense * defense_multiplier))`

Arithmetic uses a 64-bit intermediate. Rules are versioned by future callers
rather than hardwired to claims about the original ROM. This is a testable
engine *contract*, not verified Phantasy Star combat behavior.

## Planned seams — not yet implemented

1. `domain/`: characters, parties, inventory, spells, battles and events.
2. `world/`: maps, movement, encounters and scriptable interaction.
3. `content/`: versioned, independently licensed content definitions.
4. `platform/`: SDL3-based input, window, rendering and audio adapters.
5. `reference/`: trace import and differential tests against verified original
   behavior; separate original-only and enhanced rulesets.

## Accuracy rules

Do not copy copyrighted game assets or ROM-generated C into Git. Do not
assume replay parity from identical screenshots. Classic mode must be verified
against original execution before being labeled authentic; enhanced mode may
deliberately diverge, with explicit tests for the intended differences.

## Stage 2: world, character and content modules

`WorldMap` is immutable to callers and validates row dimensions, allowed
terrain (`.` floor, `#` wall), and walkable starting location. `WorldState`
tracks position/facing and returns typed outcomes for attempted movement.
Directions outside the enum are rejected without mutation. The initial
movement model is *top-down*, not a claim about authentic dungeon movement.

`Character` owns stats, cumulative XP, and level progression. The illustrative
threshold for the next level is `10 * current_level^2` cumulative XP, with
level 99 and XP 1,000,000,000 caps. Every definition is validated through its
whole possible level range to prevent stat overflow. `Party` provides a
maximum of four distinct characters, with individually mutable progression.

`content.cpp` strictly reads the versioned `PSMAP 1` and `PSCHAR 1` text
schemas from streams. It enforces identifiers, ordering, numeric ranges, tile
vocabularies, required values, full-level stat safety, and rejects unknown
fields. Game content remains independent of the runtime and is *not* imported
from the proprietary ROM. Both content formats deliberately start simple and
can be superseded later by a separately versioned JSON/TOML codec.

The new end-to-end test uses non-copyright sample content and demonstrates
content -> movement -> party -> battle -> progression without platform code.

## Stage 3: inventory, equipment, and event/quest foundations

- `items.hpp` owns versioned-content-independent `ItemDefinition`, `ItemCatalog`,
  stack-and-slot-limited `Inventory`, three-slot `Loadout`, combat stat modifiers,
  and healing consumables. Item IDs rather than ROM bank pointers identify game
  objects. Failed equip/unequip operations stage an inventory copy before commit.
- `character.hpp` now exposes bounded `heal` and `take_damage` methods. The
  existing combat prototype **does not automatically synchronize** its private
  `Actor` HP with the party; callers must apply a combat event's damage explicitly
  until a future battle-controller integration is implemented.
- `story.hpp` manages `EventEngine`, `QuestJournal`, and `WorldInteractions`.
  Event actions include story flags, item grants and XP grants; registration
  validates action shapes. When fired, the engine copies party, inventory and
  progress flags, applies actions in order, and commits all results by swaps.
  Unknown targets, unavailable inventory room or XP cap errors roll back all
  actions. One-shot flags prevent double rewards; repeatable events remain
  bounded by inventory/stat constraints.
- `content.cpp` adds `PSITEM 1`, `PSEVENT 1`, and `PSQUEST 1`. The same strict
  parser rejects unsupported schemas, invalid identifiers, bad numeric fields,
  unknown action types, incomplete documents and trailing input.
- `gameplay_flow_tests.cpp` demonstrates **illustrative** content file loading,
  moving to a cache, acquiring/equipping a weapon, completing a battle,
  consuming a healing item and advancing a quest/XP reward. It is not a full
  game loop or an original-ROM parity test.

All rules and sample assets in this stage are new modern-engine examples, not
verified or copied from the original Sega game. The original recompilation
lives at the repository root and is unchanged by this workstream.

## Stage 4: merchant, NPC and persistence boundaries

`Wallet` holds nonnegative currency capped at 1,000,000,000. A `Shop` validates
all `ShopOffer` item IDs against an `ItemCatalog` at construction. Each trade
stages inventory and wallet copies and commits both only after all checks
succeed. Negative quantities, unaffordable purchases, full inventories,
oversized price totals, excess proceeds, missing stock and key-item offers
are rejected. Shops currently have unlimited stock; equipped items live outside
inventory and cannot be sold without unequipping.

`DialogueEngine` selects a mandatory fallback dialogue line when no conditional
story flag matches. Later eligible lines override earlier eligible lines. NPC
locations are explicitly bound to walkable tiles and queried through
`talk_here`. Story flag values remain owned exclusively by `EventEngine`.

`GameState` joins world, party, inventory, per-member equipment, story state
and wallet in a runtime-owned aggregate. `encode_save` emits a deterministic
`PSAVE 1` text snapshot with a CRC32 checksum; `decode_save` validates the
whole document before returning a new state. It rejects mismatched character
rosters/map IDs/inventory capacities and unknown original content references.
The caller must reconstruct an identical content blueprint (map, party member
identities/definitions, event definitions) on the next run. Definition assets,
NPCs, shops, quest definitions, combat history and encounter RNG are *not*
serialized. See `PERSISTENCE.md` for constraints and upgrade strategy.
