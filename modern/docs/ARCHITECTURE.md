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
