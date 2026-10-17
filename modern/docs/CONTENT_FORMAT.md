# Modern Phantasy Star content — provisional v1 schema

These **original training examples** are not extracted from Sega's game. A
content package may supply its own maps and characters without any SMS ROM.
Only the documented keys and order are accepted. The parser uses CRLF or LF,
accepts no extra lines (including comments/blank lines), and throws on malformed
or unsupported content. All keys and IDs use ASCII; `name` is a printable UTF-8
byte string (UTF-8 validity itself is not yet verified).

## Map: `PSMAP 1`

Example at `content/training_ground.psmap`:

    PSMAP 1
    id=training_ground
    width=8
    height=6
    spawn_x=1
    spawn_y=1
    row=########
    row=#......#
    row=#.###..#
    row=#...#..#
    row=#......#
    row=########

- `id`: lowercase `[a-z][a-z0-9_]*`, maximum 48 characters.
- `width`, `height`: each 1..128 inclusive. Exactly `height` rows must follow.
- `spawn_x`, `spawn_y`: zero-based, inside dimensions, must select a `.` tile.
- `row`: exactly `width` characters; `.` walkable, `#` solid.
- Each `WorldState` starts at its map's spawn. Attempted moves return a typed
  `none`, `wall`, or `bounds` reason. Even blocked attempts change facing.

## Character: `PSCHAR 1`

Example at `content/training_hero.pschar`:

    PSCHAR 1
    id=training_hero
    name=Training Hero
    max_hp=30
    attack=9
    defense=4
    speed=5
    growth_hp=4
    growth_attack=2
    growth_defense=1
    growth_speed=1

- `id`: same normalized ASCII scheme as map identifiers; `name`: 1..80 bytes,
  no ASCII control characters.
- Initial `max_hp` must be positive; all remaining stats and growth rates are
  nonnegative integers; values cannot exceed 1,000,000 at level 99.
- Starting level = 1; cumulative threshold for each next level is
  `10 * current_level^2`, maximum level = 99. XP cannot exceed 1,000,000,000.
- HP increases by the max-HP growth at each level. No skills, equipment,
  healing, saving or permanent damage integration is implemented yet.
- Parties can contain at most four distinct `id` values.

## Compatibility and constraints

Use a new schema number for any incompatible format. The parser intentionally
fails closed on unknown versions, fields, out-of-range values or truncated
files. These test mechanics are **not** representations of authentic original
Phantasy Star map data, leveling formulas or enemy statistics. They will later
support both reconstructed classic and deliberately enhanced rules, each with
separate validation evidence.

## Items: `PSITEM 1`

Example: `content/training_tonic.psitem`. Exactly these fields in order:

```
PSITEM 1
id=training_tonic
name=Training Tonic
kind=consumable
max_stack=9
healing=8
attack_bonus=0
defense_bonus=0
speed_bonus=0
```

- `id`: lowercase ASCII `[a-z][a-z0-9_]*`, at most 48 bytes.
- `name`: 1..80 printable bytes. UTF-8 byte validity is not independently checked.
- `kind`: one of `consumable`, `weapon`, `armor`, `accessory`, `key_item`.
- `max_stack`: 1..99 for consumables, **exactly 1** for other item kinds.
- `healing`: 1..1,000,000 for consumables, zero otherwise.
- `attack_bonus`, `defense_bonus`, `speed_bonus`: 0..1,000,000 for equipment;
  zero for consumables and key items.
- The inventory is slot-capped by **distinct item IDs**; stack caps apply per ID.
  Equipping transfers the item out of inventory; unequipping transfers it back.
  Failed operations cannot change item or equipment state.

## Events: `PSEVENT 1`

Example: `content/training_cache.psevent`:

```
PSEVENT 1
id=training_cache
requires_flag=
repeatable=0
action_count=3
action=set_flag:found_cache:1
action=give_item:training_tonic:2
action=give_item:training_blade:1
```

- `id` and nonempty `requires_flag`: normalized identifiers. Blank prerequisite
  means the event is available immediately.
- `repeatable`: `0` or `1`. An event marked `0` runs no more than once.
- `action_count`: 1..16; exactly this many `action=` lines must follow.
- Actions: `set_flag:flag_id:1`, `give_item:item_id:positive_quantity`,
  `grant_experience:party_member_id:positive_amount`. Amounts are at most
  1,000,000. Unknown item/character references fail on attempted execution,
  because content packages may load their declarations in any order.
- The engine executes the complete list on temporary party, inventory and
  story state. Only when **all** actions succeed does the event commit.
- Events can be bound to exactly one walkable map tile at a time by
  `WorldInteractions`; `interact_here` requires occupying that tile, not mere
  proximity or automatic movement into it.

## Quests: `PSQUEST 1`

Example: `content/training_quest.psquest`:

```
PSQUEST 1
id=training_quest
title=Explore the training room
activation_flag=found_cache
completion_flag=cleared_room
```

- All flag IDs are normalized and the two flags must differ.
- `QuestJournal` reports `unavailable`, `active`, or `completed` from event
  flags; quest completion has priority over activation.
- Quest progress currently lives in memory. Persistent save/load, scriptable
  dialogue, quest rewards, trade, item durability, combat status effects,
  localization and distributed event routing **are not implemented**.

This is an intentionally small, strictly versioned native content foundation,
not original *Phantasy Star* game data. Unknown versions and extra fields are
rejected rather than silently ignored.
