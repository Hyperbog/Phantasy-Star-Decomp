#pragma once

#include "modernps/combat.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace modernps {

struct GrowthRates {
    int max_hp;
    int attack;
    int defense;
    int speed;
};

struct CharacterDefinition {
    std::string id;
    std::string display_name;
    Stats base;
    GrowthRates growth;
};

// Prototype progression curve; NOT verified against Phantasy Star's ROM.
class Character {
public:
    static constexpr int max_level = 99;
    static constexpr int max_experience = 1'000'000'000;
    explicit Character(CharacterDefinition definition);
    const CharacterDefinition& definition() const noexcept { return definition_; }
    int level() const noexcept { return level_; }
    int experience() const noexcept { return experience_; }
    const Stats& stats() const noexcept { return stats_; }
    int hp() const noexcept { return hp_; }
    int grant_experience(int amount);
    Actor as_combat_actor() const;
private:
    static int required_experience_for_next_level(int current_level) noexcept;
    CharacterDefinition definition_;
    int level_ = 1;
    int experience_ = 0;
    Stats stats_;
    int hp_;
};

// Party membership is managed independently of ROM RAM layout.
class Party {
public:
    static constexpr std::size_t max_members = 4;
    explicit Party(Character leader);
    const std::vector<Character>& members() const noexcept { return members_; }
    bool contains(std::string_view id) const noexcept;
    void add(Character member);
    Character& member(std::string_view id);
    const Character& member(std::string_view id) const;
private:
    std::vector<Character> members_;
};

} // namespace modernps
