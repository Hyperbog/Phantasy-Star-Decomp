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
