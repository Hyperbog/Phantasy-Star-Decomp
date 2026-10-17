#include "modernps/items.hpp"

#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace modernps;
namespace {
void require(bool yes, const char* msg) { if (!yes) throw std::runtime_error(msg); }
template<class Error, class Fn> void rejects(Fn&& fn) {
    bool caught = false;
    try { fn(); } catch (const Error&) { caught = true; }
    require(caught, "required exception not thrown");
}
Character hero() { return Character({"hero", "Hero", {30, 9, 4, 5}, {4, 2, 1, 1}}); }
ItemCatalog catalog() {
    ItemCatalog c;
    c.add({"tonic", "Tonic", ItemKind::consumable, 5, 8, 0, 0, 0});
    c.add({"blade", "Blade", ItemKind::weapon, 1, 0, 3, 0, 0});
    c.add({"club", "Club", ItemKind::weapon, 1, 0, 1, 0, 0});
    c.add({"mail", "Mail", ItemKind::armor, 1, 0, 0, 2, 0});
    c.add({"seal", "Seal", ItemKind::key_item, 1, 0, 0, 0, 0});
    return c;
}
void stack_limits() {
    auto c = catalog(); Inventory inv(2);
    inv.add(c, "tonic", 3); inv.add(c, "tonic", 2);
    require(inv.count("tonic") == 5 && inv.slots_used() == 1, "stack invalid");
    rejects<std::invalid_argument>([&] { inv.add(c, "tonic", 1); });
    rejects<std::invalid_argument>([&] { inv.add(c, "tonic", -1); });
    rejects<std::invalid_argument>([&] { inv.remove("tonic", 6); });
    require(inv.count("tonic") == 5, "failed operation changed stack");
    inv.remove("tonic", 5);
    require(inv.count("tonic") == 0 && inv.slots_used() == 0, "empty stack not erased");
}
void slot_caps() {
    auto c = catalog(); Inventory inv(1);
    inv.add(c, "blade", 1);
    rejects<std::invalid_argument>([&] { inv.add(c, "tonic", 1); });
    require(inv.count("blade") == 1 && inv.count("tonic") == 0, "capacity rollback failed");
    rejects<std::invalid_argument>([&] { inv.remove("blade", 0); });
}
void missing_and_invalid_kinds() {
    auto c = catalog(); Inventory inv;
    rejects<std::out_of_range>([&] { inv.add(c, "unknown", 1); });
    rejects<std::invalid_argument>([&] { inv.remove("missing", 1); });
    rejects<std::invalid_argument>([&] { Inventory bad(0); });
    rejects<std::invalid_argument>([&] { Inventory bad(1001); });
    rejects<std::invalid_argument>([&] {
        c.add({"bad", "Bad", static_cast<ItemKind>(127), 1, 0, 0, 0, 0});
    });
    rejects<std::invalid_argument>([&] {
        c.add({"seal", "Duplicate", ItemKind::key_item, 1, 0, 0, 0, 0});
    });
}
void item_definition_rules() {
    ItemCatalog c;
    rejects<std::invalid_argument>([&] { c.add({"Bad", "Name", ItemKind::key_item, 1, 0, 0, 0, 0}); });
    rejects<std::invalid_argument>([&] { c.add({"a", "", ItemKind::key_item, 1, 0, 0, 0, 0}); });
    rejects<std::invalid_argument>([&] { c.add({"a", "A", ItemKind::consumable, 2, 0, 0, 0, 0}); });
    rejects<std::invalid_argument>([&] { c.add({"a", "A", ItemKind::weapon, 2, 0, 0, 0, 0}); });
    rejects<std::invalid_argument>([&] { c.add({"a", "A", ItemKind::key_item, 1, 0, 1, 0, 0}); });
    rejects<std::invalid_argument>([&] { c.add({"a", "A", ItemKind::armor, 1, 0, -1, 0, 0}); });
    require(!c.contains("a"), "failed definitions inserted entries");
}
void equip_and_swap() {
    auto c = catalog(); Inventory inv(2);
    inv.add(c, "blade", 1); inv.add(c, "club", 1);
    Loadout gear;
    gear.equip(c, inv, "blade");
    require(inv.count("blade") == 0 && gear.equipped(ItemKind::weapon) == "blade", "equip failed");
    gear.equip(c, inv, "club");
    require(inv.count("blade") == 1 && inv.count("club") == 0 &&
            gear.equipped(ItemKind::weapon) == "club", "swap failed");
    require(gear.as_combat_actor(hero(), c).stats.attack == 10, "equip stats wrong");
    gear.unequip(c, inv, ItemKind::weapon);
    require(!gear.equipped(ItemKind::weapon) && inv.count("club") == 1, "unequip failed");
    rejects<std::logic_error>([&] { gear.unequip(c, inv, ItemKind::weapon); });
}
void equip_failure_is_atomic() {
    auto c = catalog(); Inventory inv(1); Loadout gear;
    inv.add(c, "blade", 1); gear.equip(c, inv, "blade");
    inv.add(c, "tonic", 1);
    rejects<std::invalid_argument>([&] { gear.unequip(c, inv, ItemKind::weapon); });
    require(gear.equipped(ItemKind::weapon) == "blade" && inv.count("tonic") == 1,
            "failed unequip mutated state");
    rejects<std::invalid_argument>([&] { gear.equip(c, inv, "tonic"); });
    rejects<std::invalid_argument>([&] { gear.equip(c, inv, "seal"); });
    require(gear.equipped(ItemKind::weapon) == "blade" && inv.count("tonic") == 1,
            "invalid equip mutated state");
}
void heal_consumption() {
    auto c = catalog(); Inventory inv;
    auto h = hero(); inv.add(c, "tonic", 2);
    h.take_damage(20);
    require(use_consumable(c, inv, "tonic", h) == 8 && h.hp() == 18 &&
            inv.count("tonic") == 1, "potion did not restore HP");
    require(use_consumable(c, inv, "tonic", h) == 8 && inv.count("tonic") == 0,
            "second potion failed");
}
void healing_preconditions() {
    auto c = catalog(); Inventory inv;
    auto h = hero(); inv.add(c, "tonic", 1); inv.add(c, "seal", 1);
    rejects<std::logic_error>([&] { use_consumable(c, inv, "tonic", h); });
    rejects<std::invalid_argument>([&] { use_consumable(c, inv, "seal", h); });
    h.take_damage(1);
    rejects<std::out_of_range>([&] { use_consumable(c, inv, "absent", h); }); // catalog rejects unknown
    require(use_consumable(c, inv, "tonic", h) == 1 && h.hp() == 30, "healing cap broken");
    require(inv.count("seal") == 1 && inv.count("tonic") == 0, "wrong item consumed");
    rejects<std::invalid_argument>([&] { h.heal(-1); });
    rejects<std::invalid_argument>([&] { h.take_damage(0); });
}
void multiple_equipment_slots() {
    auto c = catalog(); Inventory inv(3); Loadout gear;
    inv.add(c, "blade", 1); inv.add(c, "mail", 1);
    gear.equip(c, inv, "blade"); gear.equip(c, inv, "mail");
    const auto stats = gear.effective_stats(hero(), c);
    require(stats.attack == 12 && stats.defense == 6 && stats.speed == 5,
            "equipment bonuses incorrect");
    rejects<std::invalid_argument>([&] { gear.equipped(ItemKind::consumable); });
}
}
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> checks = {
        {"stack_limits",stack_limits}, {"slot_caps",slot_caps},
        {"missing_and_invalid_kinds",missing_and_invalid_kinds},
        {"item_definition_rules",item_definition_rules},
        {"equip_and_swap",equip_and_swap}, {"equip_failure_is_atomic",equip_failure_is_atomic},
        {"heal_consumption",heal_consumption}, {"healing_preconditions",healing_preconditions},
        {"multiple_equipment_slots",multiple_equipment_slots}
    };
    try {
        for (const auto& [name, test] : checks) { test(); std::cout << "PASS " << name << '\n'; }
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
    return 0;
}
