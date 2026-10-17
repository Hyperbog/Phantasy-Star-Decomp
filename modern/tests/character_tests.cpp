#include "modernps/character.hpp"

#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace modernps;
namespace {
void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}
template <typename Error, typename Fn>
void rejects(Fn&& op) {
    bool caught = false;
    try { op(); } catch (const Error&) { caught = true; }
    require(caught, "invalid operation accepted");
}
CharacterDefinition training() {
    return {"training_hero", "Training Hero", {30, 9, 4, 5}, {4, 2, 1, 1}};
}
void begins_at_level_one() {
    Character hero(training());
    require(hero.level() == 1 && hero.experience() == 0 && hero.hp() == 30,
            "initial character state incorrect");
    const auto actor = hero.as_combat_actor();
    require(actor.id == "training_hero" && actor.hp == 30 && actor.stats.attack == 9,
            "combat conversion incorrect");
}
void experience_threshold() {
    Character hero(training());
    require(hero.grant_experience(9) == 0 && hero.level() == 1, "early level-up");
    require(hero.grant_experience(1) == 1 && hero.level() == 2,
            "expected level 2 at 10 XP");
    require(hero.hp() == 34 && hero.stats().attack == 11,
            "stat growth incorrect");
    require(hero.grant_experience(29) == 0, "threshold not reached yet");
    require(hero.grant_experience(1) == 1 && hero.level() == 3,
            "expected level 3 at 40 XP");
}
void multiple_levels() {
    Character hero(training());
    require(hero.grant_experience(100) == 3 && hero.level() == 4,
            "multi-level progression incorrect");
    require(hero.hp() == 42 && hero.stats().speed == 8,
            "multi-level stat growth incorrect");
}
void level_cap() {
    Character hero(training());
    require(hero.grant_experience(100'000) == 98 && hero.level() == 99,
            "maximum character level incorrect");
    require(hero.stats().max_hp == 422 && hero.hp() == 422,
            "max-level HP growth incorrect");
    require(hero.grant_experience(100'000) == 0 && hero.level() == 99,
            "exceeding level cap");
}
void bad_experience_is_atomic() {
    Character hero(training());
    rejects<std::invalid_argument>([&] { hero.grant_experience(-1); });
    require(hero.level() == 1 && hero.experience() == 0,
            "negative gain changed state");
    hero.grant_experience(Character::max_experience);
    rejects<std::invalid_argument>([&] { hero.grant_experience(1); });
    require(hero.experience() == Character::max_experience && hero.level() == 99,
            "overflow changed state");
}
void party_management() {
    Party party{Character(training())};
    require(party.members().size() == 1 && party.contains("training_hero"),
            "party leader missing");
    auto ally = training(); ally.id = "ally";
    party.add(Character(ally));
    require(party.members().size() == 2 && party.contains("ally"),
            "party add failed");
    party.member("ally").grant_experience(10);
    require(party.member("ally").level() == 2 &&
            party.member("training_hero").level() == 1,
            "party member progression not independent");
    const Party& readonly = party;
    require(readonly.member("ally").level() == 2,
            "const party lookup failed");
    rejects<std::invalid_argument>([&] { party.add(Character(ally)); });
    require(party.members().size() == 2, "duplicate add changed party");
    rejects<std::out_of_range>([&] { party.member("absent"); });
    ally.id = "third"; party.add(Character(ally));
    ally.id = "fourth"; party.add(Character(ally));
    require(party.members().size() == Party::max_members,
            "party should have four members");
    ally.id = "fifth";
    rejects<std::invalid_argument>([&] { party.add(Character(ally)); });
    require(party.members().size() == Party::max_members,
            "overfull add changed party");
}
void invalid_definition() {
    auto def = training();
    def.base.max_hp = 0;
    rejects<std::invalid_argument>([&] { Character invalid(def); });
    def = training();
    def.growth.attack = -1;
    rejects<std::invalid_argument>([&] { Character invalid(def); });
    def = training();
    def.base.attack = std::numeric_limits<int>::max();
    rejects<std::invalid_argument>([&] { Character invalid(def); });
    def = training();
    def.id.clear();
    rejects<std::invalid_argument>([&] { Character invalid(def); });
}
} // namespace
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> checks = {
        {"begins_at_level_one", begins_at_level_one},
        {"experience_threshold", experience_threshold},
        {"multiple_levels", multiple_levels}, {"level_cap", level_cap},
        {"bad_experience_is_atomic", bad_experience_is_atomic},
        {"invalid_definition", invalid_definition},
        {"party_management", party_management}
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
