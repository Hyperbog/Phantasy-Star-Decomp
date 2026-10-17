#include "modernps/items.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace modernps {

bool valid_content_id(std::string_view id) noexcept {
    if (id.empty() || id.size() > 48 || id.front() < 'a' || id.front() > 'z') return false;
    return std::all_of(id.begin(), id.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
    });
}

void ItemCatalog::add(ItemDefinition definition) {
    if (!valid_content_id(definition.id) || definition.name.empty() ||
        definition.name.size() > 80 ||
        std::any_of(definition.name.begin(), definition.name.end(), [](unsigned char c) {
            return c < 0x20 || c == 0x7f;
        }) || definition.max_stack < 1 || definition.max_stack > 99 ||
        definition.healing < 0 || definition.healing > 1'000'000 ||
        definition.attack_bonus < 0 || definition.attack_bonus > 1'000'000 ||
        definition.defense_bonus < 0 || definition.defense_bonus > 1'000'000 ||
        definition.speed_bonus < 0 || definition.speed_bonus > 1'000'000) {
        throw std::invalid_argument("invalid item definition");
    }
    const bool modifiers = definition.attack_bonus || definition.defense_bonus ||
                           definition.speed_bonus;
    switch (definition.kind) {
        case ItemKind::consumable:
            if (!definition.healing || modifiers) throw std::invalid_argument("invalid consumable fields");
            break;
        case ItemKind::weapon:
        case ItemKind::armor:
        case ItemKind::accessory:
            if (definition.max_stack != 1 || definition.healing)
                throw std::invalid_argument("invalid equipment fields");
            break;
        case ItemKind::key_item:
            if (definition.max_stack != 1 || definition.healing || modifiers)
                throw std::invalid_argument("invalid key item fields");
            break;
        default: throw std::invalid_argument("unknown item kind");
    }
    const auto id = definition.id;
    if (!items_.emplace(id, std::move(definition)).second)
        throw std::invalid_argument("duplicate item id");
}

const ItemDefinition& ItemCatalog::get(std::string_view id) const {
    const auto found = items_.find(id);
    if (found == items_.end()) throw std::out_of_range("item not in catalog");
    return found->second;
}

bool ItemCatalog::contains(std::string_view id) const noexcept {
    return items_.find(id) != items_.end();
}

Inventory::Inventory(std::size_t max_slots) : max_slots_(max_slots) {
    if (max_slots < 1 || max_slots > 1000) throw std::invalid_argument("invalid inventory capacity");
}

int Inventory::count(std::string_view id) const noexcept {
    const auto found = quantities_.find(id);
    return found == quantities_.end() ? 0 : found->second;
}

void Inventory::add(const ItemCatalog& catalog, std::string_view id, int quantity) {
    const auto& item = catalog.get(id);
    if (quantity < 1 || static_cast<std::int64_t>(count(id)) + quantity > item.max_stack ||
        (count(id) == 0 && quantities_.size() >= max_slots_)) {
        throw std::invalid_argument("inventory has insufficient capacity");
    }
    // Insertion may allocate, but it cannot mutate an existing entry on failure.
    quantities_[item.id] += quantity;
}

void Inventory::remove(std::string_view id, int quantity) {
    const auto found = quantities_.find(id);
    if (quantity < 1 || found == quantities_.end() || found->second < quantity)
        throw std::invalid_argument("item quantity unavailable");
    found->second -= quantity;
    if (found->second == 0) quantities_.erase(found);
}

void Inventory::swap(Inventory& other) noexcept {
    using std::swap;
    swap(max_slots_, other.max_slots_);
    quantities_.swap(other.quantities_);
}

std::size_t Loadout::slot(ItemKind kind) {
    switch (kind) {
        case ItemKind::weapon: return 0;
        case ItemKind::armor: return 1;
        case ItemKind::accessory: return 2;
        default: throw std::invalid_argument("item kind is not equippable");
    }
}

const std::optional<std::string>& Loadout::equipped(ItemKind kind) const {
    return slots_[slot(kind)];
}

void Loadout::equip(const ItemCatalog& catalog, Inventory& inventory,
                    std::string_view id) {
    const auto& item = catalog.get(id);
    const auto index = slot(item.kind);
    Inventory staged_inventory = inventory;
    Loadout staged_loadout = *this;
    staged_inventory.remove(item.id, 1);
    if (staged_loadout.slots_[index])
        staged_inventory.add(catalog, *staged_loadout.slots_[index], 1);
    staged_loadout.slots_[index] = item.id;
    inventory.swap(staged_inventory);
    slots_.swap(staged_loadout.slots_);
}

void Loadout::unequip(const ItemCatalog& catalog, Inventory& inventory,
                      ItemKind kind) {
    const auto index = slot(kind);
    if (!slots_[index]) throw std::logic_error("equipment slot is empty");
    Inventory staged_inventory = inventory;
    staged_inventory.add(catalog, *slots_[index], 1);
    inventory.swap(staged_inventory);
    slots_[index].reset();
}

Loadout Loadout::restore(const ItemCatalog& catalog,
                         const std::array<std::optional<std::string>, 3>& slots) {
    Loadout loadout;
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (!slots[i]) continue;
        const auto& definition = catalog.get(*slots[i]);
        if (slot(definition.kind) != i)
            throw std::invalid_argument("restored item in incorrect equipment slot");
        loadout.slots_[i] = definition.id;
    }
    return loadout;
}

Stats Loadout::effective_stats(const Character& character,
                               const ItemCatalog& catalog) const {
    Stats result = character.stats();
    for (const auto& id : slots_) {
        if (!id) continue;
        const auto& item = catalog.get(*id);
        const auto add = [](int a, int b) {
            const auto sum = static_cast<std::int64_t>(a) + b;
            if (sum > std::numeric_limits<int>::max())
                throw std::overflow_error("equipment exceeds stat range");
            return static_cast<int>(sum);
        };
        result.attack = add(result.attack, item.attack_bonus);
        result.defense = add(result.defense, item.defense_bonus);
        result.speed = add(result.speed, item.speed_bonus);
    }
    return result;
}

Actor Loadout::as_combat_actor(const Character& character,
                               const ItemCatalog& catalog) const {
    return {character.definition().id, effective_stats(character, catalog), character.hp()};
}

int use_consumable(const ItemCatalog& catalog, Inventory& inventory,
                   std::string_view id, Character& target) {
    const auto& item = catalog.get(id);
    if (item.kind != ItemKind::consumable) throw std::invalid_argument("item not consumable");
    if (target.hp() == target.stats().max_hp) throw std::logic_error("target already healthy");
    Inventory staged = inventory;
    staged.remove(id, 1);
    const int healed = target.heal(item.healing);
    inventory.swap(staged);
    return healed;
}

} // namespace modernps
