#include "modernps/content.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace modernps;
namespace {
void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}
const std::string map_text =
    "PSMAP 1\nid=demo\nwidth=3\nheight=3\nspawn_x=1\nspawn_y=1\n"
    "row=###\nrow=#.#\nrow=###\n";
const std::string character_text =
    "PSCHAR 1\nid=demo\nname=Demo Hero\nmax_hp=30\nattack=9\n"
    "defense=4\nspeed=5\ngrowth_hp=4\ngrowth_attack=2\n"
    "growth_defense=1\ngrowth_speed=1\n";

template <typename Factory>
void rejects(const std::string& document, Factory factory) {
    std::istringstream input(document);
    bool threw = false;
    try { factory(input); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "malformed content unexpectedly accepted");
}
void embedded_map() {
    std::istringstream input(map_text);
    WorldState state(load_world_map(input));
    require(state.map().id() == "demo" && state.position() == Position{1, 1},
            "parsed map incorrect");
    require(state.move(Direction::east).blocked_by == MoveBlock::wall,
            "parsed collision not applied");
}
void embedded_character() {
    std::istringstream input(character_text);
    Character hero(load_character_definition(input));
    require(hero.definition().display_name == "Demo Hero" &&
            hero.grant_experience(10) == 1 && hero.stats().attack == 11,
            "parsed character progression incorrect");
}
void reject_version() {
    auto changed = map_text;
    changed.replace(0, 7, "PSMAP 2");
    rejects(changed, load_world_map);
    changed = character_text;
    changed.replace(0, 8, "PSCHAR 2");
    rejects(changed, load_character_definition);
}
void reject_unknown_and_duplicate_fields() {
    rejects(map_text + "extra=oops\n", load_world_map);
    rejects(character_text + "attack=100\n", load_character_definition);
    auto changed = map_text;
    changed.replace(changed.find("height=3"), 8, "weight=3");
    rejects(changed, load_world_map);
}
void reject_invalid_tile_or_spawn() {
    auto changed = map_text;
    changed.replace(changed.find("row=#.#"), 7, "row=#X#");
    rejects(changed, load_world_map);
    changed = map_text;
    changed.replace(changed.find("row=#.#"), 7, "row=###");
    rejects(changed, load_world_map);
    changed = map_text;
    changed.replace(changed.find("spawn_x=1"), 9, "spawn_x=9");
    rejects(changed, load_world_map);
}
void reject_truncation_and_numbers() {
    rejects("PSMAP 1\nid=demo\nwidth=3\n", load_world_map);
    auto changed = map_text;
    changed.replace(changed.find("width=3"), 7, "width=3x");
    rejects(changed, load_world_map);
    changed = character_text;
    changed.replace(changed.find("attack=9"), 8, "attack=-2");
    rejects(changed, load_character_definition);
    changed = character_text;
    changed.replace(changed.find("growth_hp=4"), 11, "growth_hp=999999");
    rejects(changed, load_character_definition);
}
void reject_bad_identifier_and_controls() {
    auto changed = character_text;
    changed.replace(changed.find("id=demo"), 7, "id=Demo");
    rejects(changed, load_character_definition);
    changed = map_text;
    changed.insert(changed.find("id=demo") + 7, "\t");
    rejects(changed, load_world_map);
}
void load_shipped_examples() {
    std::ifstream map_file(std::string(MODERNPS_CONTENT_DIR) + "/training_ground.psmap");
    std::ifstream char_file(std::string(MODERNPS_CONTENT_DIR) + "/training_hero.pschar");
    require(map_file.good() && char_file.good(), "example content files missing");
    WorldState state(load_world_map(map_file));
    Character hero(load_character_definition(char_file));
    require(state.map().width() == 8 && state.map().height() == 6 &&
            state.position() == Position{1, 1}, "shipped map incorrect");
    require(state.move(Direction::east).moved(), "shipped map cannot be navigated");
    require(hero.as_combat_actor().stats.max_hp == 30,
            "shipped character cannot initialize battle data");
}
} // namespace
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> checks = {
        {"embedded_map", embedded_map},
        {"embedded_character", embedded_character},
        {"reject_version", reject_version},
        {"reject_unknown_and_duplicate_fields", reject_unknown_and_duplicate_fields},
        {"reject_invalid_tile_or_spawn", reject_invalid_tile_or_spawn},
        {"reject_truncation_and_numbers", reject_truncation_and_numbers},
        {"reject_bad_identifier_and_controls", reject_bad_identifier_and_controls},
        {"load_shipped_examples", load_shipped_examples}
    };
    try {
        for (const auto& [name, check] : checks) {
            check();
            std::cout << "PASS " << name << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
