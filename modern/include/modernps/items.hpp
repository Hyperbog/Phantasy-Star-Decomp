#pragma once

#include "modernps/character.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace modernps {

// These are engine-owned, illustrative items, NOT extracted ROM definitions.
enum class ItemKind { consumable, weapon, armor, accessory, key_item };

struct ItemDefinition {
    std::string id;
    std::string name;
    ItemKind kind;
    int max_stack;
    int healing;
    int attack_bonus;
    int defense_bonus;
    int speed_bonus;
};

bool valid_content_id(std::string_view id) noexcept;

class ItemCatalog {
public:
    void add(ItemDefinition definition);
    const ItemDefinition& get(std::string_view id) const;
    bool contains(std::string_view id) const noexcept;
private:
    std::map<std::string, ItemDefinition, std::less<>> items_;
};

class Inventory {
public:
    explicit Inventory(std::size_t max_slots = 20);
    std::size_t slots_used() const noexcept { return quantities_.size(); }
    std::size_t capacity() const noexcept { return max_slots_; }
    int count(std::string_view id) const noexcept;
    void add(const ItemCatalog& catalog, std::string_view id, int quantity);
    void remove(std::string_view id, int quantity);
    void swap(Inventory& other) noexcept;
private:
    std::size_t max_slots_;
    std::map<std::string, int, std::less<>> quantities_;
};

class Loadout {
public:
    const std::optional<std::string>& equipped(ItemKind kind) const;
    void equip(const ItemCatalog& catalog, Inventory& inventory,
               std::string_view id);
    void unequip(const ItemCatalog& catalog, Inventory& inventory,
                 ItemKind kind);
    Stats effective_stats(const Character& character,
                          const ItemCatalog& catalog) const;
    Actor as_combat_actor(const Character& character,
                          const ItemCatalog& catalog) const;
private:
    static std::size_t slot(ItemKind kind);
    std::array<std::optional<std::string>, 3> slots_{};
};

// Returns actual HP restored; refuses to consume if target is at full HP.
int use_consumable(const ItemCatalog& catalog, Inventory& inventory,
                   std::string_view id, Character& target);

} // namespace modernps
