#pragma once

#include <optional>
#include <string>

namespace modernps {

// Engine-owned model. Values are NOT assertions about the 1987 game ROM.
struct Stats {
    int max_hp;
    int attack;
    int defense;
    int speed;
};

struct Actor {
    std::string id;
    Stats stats;
    int hp;
};

enum class Side { player, enemy };

// Replace with ROM-derived behavior only after independent replay evidence.
struct DamageRules {
    int minimum_damage = 1;
    int attack_multiplier = 1;
    int defense_multiplier = 1;
};

struct AttackEvent {
    Side attacker;
    std::string attacker_id;
    std::string target_id;
    int damage;
    int target_hp_before;
    int target_hp_after;
    bool battle_finished;
};

// A deterministic domain model: no renderer, random number generator,
// platform library, ROM, or SMS register state required.
class Battle {
public:
    Battle(Actor player, Actor enemy, DamageRules rules = {});

    const Actor& player() const noexcept { return player_; }
    const Actor& enemy() const noexcept { return enemy_; }
    std::optional<Side> next_turn() const noexcept;
    std::optional<Side> winner() const noexcept;
    bool finished() const noexcept;

    // Enforces turn order. Invalid actions never modify either actor.
    AttackEvent attack(Side side);

private:
    Actor player_;
    Actor enemy_;
    DamageRules rules_;
    Side turn_ = Side::player;
};

}  // namespace modernps
