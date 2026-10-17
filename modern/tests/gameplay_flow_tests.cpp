#include "modernps/content.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace modernps;
namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
}
int main() {
    try {
        const std::string directory = MODERNPS_CONTENT_DIR;
        auto open = [&](const char* filename) {
            std::ifstream file(directory + "/" + filename);
            if (!file) throw std::runtime_error("example content missing");
            return file;
        };
        auto map_file = open("training_ground.psmap");
        auto hero_file = open("training_hero.pschar");
        auto enemy_file = open("training_slime.pschar");
        auto tonic_file = open("training_tonic.psitem");
        auto blade_file = open("training_blade.psitem");
        auto cache_file = open("training_cache.psevent");
        auto gate_file = open("training_gate.psevent");
        auto quest_file = open("training_quest.psquest");

        WorldState world(load_world_map(map_file));
        Party party(Character(load_character_definition(hero_file)));
        Character enemy(load_character_definition(enemy_file));
        ItemCatalog items;
        items.add(load_item_definition(tonic_file));
        items.add(load_item_definition(blade_file));
        Inventory inventory(3);
        Loadout loadout;
        EventEngine events;
        events.add_event(load_event_definition(cache_file));
        events.add_event(load_event_definition(gate_file));
        QuestJournal journal;
        journal.add_quest(load_quest_definition(quest_file));
        WorldInteractions tiles;
        tiles.bind(world.map(), {2, 1}, "training_cache", events);
        tiles.bind(world.map(), {3, 1}, "training_gate", events);
        check(journal.status("training_quest", events) == QuestStatus::unavailable,
              "quest should not be active before interacting");
        check(world.move(Direction::east).moved(), "could not reach cache");
        check(world.move(Direction::south).blocked_by == MoveBlock::wall,
              "blocked wall incorrectly traversed");
        check(tiles.interact_here(world, events, party, inventory, items) == EventResult::applied,
              "initial cache event did not fire");
        check(inventory.count("training_tonic") == 2 &&
              inventory.count("training_blade") == 1 &&
              journal.status("training_quest", events) == QuestStatus::active,
              "quest/inventory state after pickup wrong");
        loadout.equip(items, inventory, "training_blade");
        check(loadout.effective_stats(party.member("training_hero"), items).attack == 11,
              "equipment bonus did not propagate to combat stats");
        Battle battle(loadout.as_combat_actor(party.member("training_hero"), items),
                      enemy.as_combat_actor());
        const auto hit = battle.attack(Side::player);
        check(hit.damage == 10 && hit.target_hp_after == 1,
              "equipped combat calculation wrong");
        const auto retaliate = battle.attack(Side::enemy);
        check(retaliate.damage == 1, "original illustrative enemy damage changed");
        party.member("training_hero").take_damage(retaliate.damage);
        check(battle.attack(Side::player).battle_finished &&
              battle.winner() == Side::player, "equipped battle did not finish");
        check(use_consumable(items, inventory, "training_tonic",
                             party.member("training_hero")) == 1 &&
              inventory.count("training_tonic") == 1,
              "post-battle healing did not consume exactly one item");
        check(world.move(Direction::east).moved(), "could not reach quest endpoint");
        check(tiles.interact_here(world, events, party, inventory, items) == EventResult::applied,
              "quest completion event did not fire");
        check(journal.status("training_quest", events) == QuestStatus::completed &&
              party.member("training_hero").level() == 2 &&
              party.member("training_hero").experience() == 10,
              "experience reward or quest completion wrong");
        check(tiles.interact_here(world, events, party, inventory, items) ==
              EventResult::already_completed, "quest rewards paid twice");
        std::cout << "PASS map->pickup->equip->battle->healing->quest->XP\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
    return 0;
}
