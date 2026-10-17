#include "modernps/combat.hpp"

#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

using modernps::Actor;
using modernps::Battle;
using modernps::DamageRules;
using modernps::Side;
using modernps::Stats;

namespace {

Actor hero() { return {"hero", {30, 9, 4, 5}, 30}; }
Actor monster() { return {"monster", {16, 6, 2, 3}, 16}; }

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Error, typename Action>
void must_throw(Action&& action) {
    bool threw_expected = false;
    try { action(); } catch (const Error&) { threw_expected = true; }
    require(threw_expected, "expected exception was not raised");
}

void initial_turn() {
    Battle battle(hero(), monster());
    require(!battle.finished(), "fresh battle must be active");
    require(!battle.winner().has_value(), "fresh battle has no winner");
    require(battle.next_turn() == Side::player, "player acts first");
}

void deterministic_round() {
    Battle battle(hero(), monster());
    auto first = battle.attack(Side::player);
    require(first.damage == 7 && first.target_hp_after == 9,
            "first attack result incorrect");
    require(battle.next_turn() == Side::enemy, "turn must alternate");
    auto second = battle.attack(Side::enemy);
    require(second.damage == 2 && battle.player().hp == 28,
            "enemy turn incorrect");
    require(battle.next_turn() == Side::player, "turn must return to player");
}

void finish_blocks_additional_actions() {
    Battle battle(hero(), {"weak", {4, 1, 0, 0}, 4});
    auto result = battle.attack(Side::player);
    require(result.damage == 4 && result.battle_finished,
            "lethal damage must be capped at remaining HP");
    require(battle.winner() == Side::player && !battle.next_turn(),
            "winner and terminal turn incorrect");
    must_throw<std::logic_error>([&] { battle.attack(Side::enemy); });
}

void illegal_turn_does_not_mutate() {
    Battle battle(hero(), monster());
    must_throw<std::logic_error>([&] { battle.attack(Side::enemy); });
    require(battle.player().hp == 30 && battle.enemy().hp == 16 &&
            battle.next_turn() == Side::player,
            "out-of-turn action changed state");
}

void configurable_rules() {
    Battle battle(hero(), monster(), DamageRules{0, 0, 1});
    const auto event = battle.attack(Side::player);
    require(event.damage == 0 && battle.enemy().hp == 16,
            "zero-damage mode incorrect");
}

void overflow_safe_damage() {
    constexpr int maximum = std::numeric_limits<int>::max();
    Battle battle({"high", {maximum, maximum, 0, 1}, maximum},
                  {"low", {maximum, 0, 0, 1}, maximum},
                  DamageRules{0, maximum, 0});
    require(battle.attack(Side::player).damage == maximum,
            "damage multiplication overflowed");
}

void invalid_actors() {
    auto h = hero();
    h.hp = 31;
    must_throw<std::invalid_argument>([&] { Battle battle(h, monster()); });
    h = hero();
    h.stats.defense = -1;
    must_throw<std::invalid_argument>([&] { Battle battle(h, monster()); });
    h = hero();
    h.id = "monster";
    must_throw<std::invalid_argument>([&] { Battle battle(h, monster()); });
}

void invalid_rules() {
    must_throw<std::invalid_argument>([] {
        Battle battle(hero(), monster(), DamageRules{-1, 1, 1});
    });
    must_throw<std::invalid_argument>([] {
        Battle battle(hero(), monster(), DamageRules{1, -1, 1});
    });
}

}  // namespace

int main() {
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"initial_turn", initial_turn},
        {"deterministic_round", deterministic_round},
        {"finish_blocks_additional_actions", finish_blocks_additional_actions},
        {"illegal_turn_does_not_mutate", illegal_turn_does_not_mutate},
        {"configurable_rules", configurable_rules},
        {"overflow_safe_damage", overflow_safe_damage},
        {"invalid_actors", invalid_actors},
        {"invalid_rules", invalid_rules},
    };
    try {
        for (const auto& [name, test] : tests) {
            test();
            std::cout << "PASS " << name << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
