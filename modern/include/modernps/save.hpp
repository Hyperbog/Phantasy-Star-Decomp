#pragma once

#include "modernps/dialogue.hpp"
#include "modernps/economy.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace modernps {

// Owns mutable campaign state. World/NPC/shop/item/event definitions are loaded
// separately; the supplied blueprint must contain the identical party roster,
// world and event definitions when restoring a snapshot.
struct GameState {
    WorldState world;
    Party party;
    Inventory inventory;
    std::map<std::string, Loadout, std::less<>> equipment;
    EventEngine events;
    Wallet wallet;
};

// Canonical, checked, ROM-independent PSAVE 1 format. Checksum detects
// accidental corruption; it is not a cryptographic signature.
std::string encode_save(const GameState& state);
GameState decode_save(std::string_view bytes, const GameState& blueprint,
                      const ItemCatalog& catalog);

// Write to a sibling temporary file and rename only after a complete write.
// Existing saves are never deleted as a fallback for a failed rename.
void write_save_file(const std::filesystem::path& path, const GameState& state);
GameState read_save_file(const std::filesystem::path& path,
                         const GameState& blueprint, const ItemCatalog& catalog);

} // namespace modernps
