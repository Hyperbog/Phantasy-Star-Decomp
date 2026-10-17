#include "modernps/content.hpp"
#include "modernps/save.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace modernps;
namespace {
void check(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
}

int main() {
    try {
        const auto root = std::string(MODERNPS_CONTENT_DIR) + '/';
        auto read = [&](const char* name) {
            std::ifstream file(root + name);
            if (!file) throw std::runtime_error("missing campaign content");
            return file;
        };
        auto map_input = read("training_ground.psmap");
        auto hero_input = read("training_hero.pschar");
        auto enemy_input = read("training_slime.pschar");
        auto tonic_input = read("training_tonic.psitem");
        auto blade_input = read("training_blade.psitem");
        auto shop_input = read("training_shop.psshop");
        auto npc_input = read("training_merchant.psnpc");
        auto cache_input = read("training_cache.psevent");
        auto gate_input = read("training_gate.psevent");
        auto quest_input = read("training_quest.psquest");

        const auto map = load_world_map(map_input);
        const auto hero = load_character_definition(hero_input);
        const auto enemy = load_character_definition(enemy_input);
        ItemCatalog catalog;
        catalog.add(load_item_definition(tonic_input));
        catalog.add(load_item_definition(blade_input));
        Shop shop(load_shop_definition(shop_input, catalog), catalog);
        DialogueEngine dialogue;
        dialogue.add_npc(load_npc_definition(npc_input));
        dialogue.bind(map, {3,1}, "training_merchant");
        EventEngine definitions;
        definitions.add_event(load_event_definition(cache_input));
        definitions.add_event(load_event_definition(gate_input));
        QuestJournal quests;
        quests.add_quest(load_quest_definition(quest_input));
        GameState blueprint{WorldState(map), Party(Character(hero)), Inventory(3),
                            {}, definitions, Wallet(0)};
        GameState state = blueprint;
        state.wallet.credit(100);
        shop.buy("training_tonic", 1, state.wallet, state.inventory, catalog);
        check(state.wallet.balance() == 88 && state.inventory.count("training_tonic") == 1,
              "cannot buy a healing potion");

        WorldInteractions interactions;
        interactions.bind(map, {2,1}, "training_cache", state.events);
        interactions.bind(map, {3,1}, "training_gate", state.events);
        check(state.world.move(Direction::east).moved(), "cannot enter cache tile");
        check(interactions.interact_here(state.world, state.events, state.party,
                                         state.inventory, catalog) == EventResult::applied,
              "cache event did not fire");
        check(quests.status("training_quest", state.events) == QuestStatus::active,
              "quest never activated");
        Loadout equipment;
        equipment.equip(catalog, state.inventory, "training_blade");
        state.equipment.emplace("training_hero", equipment);

        Battle battle(state.equipment.at("training_hero").as_combat_actor(
                          state.party.member("training_hero"), catalog),
                      Character(enemy).as_combat_actor());
        check(battle.attack(Side::player).damage == 10, "equipment not used in combat");
        const auto counter = battle.attack(Side::enemy);
        state.party.member("training_hero").take_damage(counter.damage);
        check(battle.attack(Side::player).battle_finished, "battle never ended");
        check(use_consumable(catalog, state.inventory, "training_tonic",
                             state.party.member("training_hero")) == 1,
              "healing failed");
        check(state.world.move(Direction::east).moved(), "cannot reach quest gate");
        check(dialogue.talk_here(state.world, state.events)->text ==
              "Welcome to the training shop.", "initial dialogue incorrect");
        check(interactions.interact_here(state.world, state.events, state.party,
                                         state.inventory, catalog) == EventResult::applied,
              "quest gate did not fire");
        check(quests.status("training_quest", state.events) == QuestStatus::completed &&
              dialogue.talk_here(state.world, state.events)->matched_flag == "cleared_room",
              "flag-driven NPC dialogue or quest completion incorrect");
        const auto serialized = encode_save(state);
        const auto resumed = decode_save(serialized, blueprint, catalog);
        check(encode_save(resumed) == serialized && resumed.wallet.balance() == 88 &&
              resumed.party.member("training_hero").level() == 2,
              "campaign restore failed");
        check(dialogue.talk_here(resumed.world, resumed.events)->matched_flag == "cleared_room" &&
              quests.status("training_quest", resumed.events) == QuestStatus::completed,
              "dialogue/quest missing after restore");
        auto after = resumed;
        shop.sell("training_tonic", 1, after.wallet, after.inventory, catalog);
        check(after.wallet.balance() == 94 && after.inventory.count("training_tonic") == 1,
              "shopping failed after restore");
        check(encode_save(state) == serialized, "restored session modified original state");
        std::cout << "PASS content->shop->explore->loot->equip->battle->heal->quest->NPC->save->resume->sell\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
    return 0;
}
