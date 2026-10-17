#include "modernps/content.hpp"
#include "modernps/combat.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace modernps;

int main() {
    try {
        const std::string base = MODERNPS_CONTENT_DIR;
        std::ifstream world_file(base + "/training_ground.psmap");
        std::ifstream player_file(base + "/training_hero.pschar");
        std::ifstream enemy_file(base + "/training_slime.pschar");
        if (!world_file || !player_file || !enemy_file) {
            throw std::runtime_error("missing sample content");
        }
        WorldState state(load_world_map(world_file));
        Party party{Character(load_character_definition(player_file))};
        Character enemy(load_character_definition(enemy_file));
        if (!state.move(Direction::east).moved() ||
            state.move(Direction::south).moved() ||
            state.position() != Position{2, 1}) {
            throw std::runtime_error("movement and wall gating failed");
        }
        Battle battle(party.member("training_hero").as_combat_actor(),
                      enemy.as_combat_actor());
        const auto hit = battle.attack(Side::player);
        if (hit.damage != 8 || hit.target_hp_after != 3) {
            throw std::runtime_error("loaded attack/defense values not used");
        }
        if (battle.attack(Side::enemy).damage != 1) {
            throw std::runtime_error("loaded enemy damage not used");
        }
        if (!battle.attack(Side::player).battle_finished ||
            battle.winner() != Side::player) {
            throw std::runtime_error("sample battle did not complete");
        }
        if (party.member("training_hero").grant_experience(10) != 1 ||
            party.member("training_hero").level() != 2 ||
            party.member("training_hero").stats().attack != 11) {
            throw std::runtime_error("post-battle progression failed");
        }
        std::cout << "PASS content->world->party->combat->progression demo\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
