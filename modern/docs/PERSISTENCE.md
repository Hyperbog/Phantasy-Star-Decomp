# Modern Phantasy Star — `PSAVE 1` persistence contract

This is a **new engine-owned** save format, NOT a reimplementation of the
original Sega Master System cartridge SRAM layout. It stores a complete
checkpoint of the **current supported mutable game state**, not an entire
replay or rendering/session/audio state.

`encode_save(GameState)` returns a deterministic, LF-terminated text document:

1. `PSAVE 1` magic; `map`, `x`, `y`, `facing` (north/east/south/west = 0..3).
2. `wallet` balance, `party` count, one `actor=id,experience,hp` per member.
3. `inventory_capacity`, `inventory` count, `item=id,quantity` records.
4. `loadouts` count and per-member `loadout=id,weapon,armor,accessory`
   records; a dash `-` marks an empty slot.
5. `flags` count with `flag=id` records, `completed` count with `event=id`.
6. `checksum=` followed by an 8-digit lowercase CRC32 over the preceding
   bytes, including the LF preceding the checksum field.

All records have a fixed order and exact counts. Decoding rejects unsupported
versions, malformed fields, invalid identifiers, impossible coordinates,
invalid or over-cap HP/XP, unknown items/events, wrong item slots, changed
party/map IDs, mismatched inventory capacity, duplicate keys, trailing data
and checksum mismatch. Entire files larger than 128 KiB are rejected.

`decode_save` creates a **separate** candidate `GameState`: errors cannot
partially mutate the old state. The caller decides when to replace its active
state. `write_save_file` writes a sibling `.tmp` first, then attempts rename;
preexisting `.tmp` files fail without overwriting. On platforms where rename
cannot atomically replace an existing target, replacement may fail and the
existing save is preserved. Filesystem `fsync`/power-loss durability is **not**
yet provided. Do not advertise crash-safe save updates on all platforms.

### Content and compatibility

To restore a saved session, load its original map, identical character roster
and definitions, original event definitions, and matching item catalog. Pass
these as a `GameState` blueprint plus item catalog. The format records roster
progress, not character definitions/recruitment metadata; the caller must
reconstruct the same party before decoding. NPC, shop, and quest definitions
are loaded separately and their state derives from persisted story flags.

There is not yet a content-version/hash compatibility manifest. A content
update that changes map geometry, item definitions, characters or event IDs
may invalidate saves or alter interpreted equipment bonuses. Before shipping,
add a content manifest, versioned migrations, and explicit compatibility tests.

CRC32 catches accidental corruption but does **not** authenticate against
adversarial edits. Input is parsed with size and count bounds, nevertheless
it should not be treated as a security boundary for untrusted mods.

### Verification

`modernps_save` tests canonical round trips, story/HP/equipment restoration,
corruption and deliberate checksum-preserving malformed documents, wrong
blueprints, zero HP, bounded disk IO, and blocked temporary-file writes.
`modernps_campaign` verifies a full in-memory gameplay sequence followed by
restoration and post-load trading. All source, sample content and saves are
original illustrative data, not ROM-derived assets or mechanics.
