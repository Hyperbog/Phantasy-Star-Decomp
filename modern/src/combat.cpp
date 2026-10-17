#include "modernps/combat.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace modernps {
namespace {

void validate_actor(const Actor& actor) {
    if (actor.id.empty() || actor.stats.max_hp <= 0 ||
        actor.hp <= 0 || actor.hp > actor.stats.max_hp ||
        actor.stats.attack < 0 || actor.stats.defense < 0 ||
        actor.stats.speed < 0) {
        throw std::invalid_argument("invalid starting actor stats or HP");
    }
}

void validate_rules(const DamageRules& rules) {
    if (rules.minimum_damage < 0 || rules.attack_multiplier < 0 ||
        rules.defense_multiplier < 0) {
        throw std::invalid_argument("damage rules cannot be negative");
    }
}

int damage_for(const Actor& source, const Actor& target,
               const DamageRules& rules) {
    // int64_t prevents overflow for products of two nonnegative int values.
    const auto offense = static_cast<std::int64_t>(source.stats.attack) *
                         rules.attack_multiplier;
    const auto protection = static_cast<std::int64_t>(target.stats.defense) *
                            rules.defense_multiplier;
    const auto raw = std::max<std::int64_t>(rules.minimum_damage,
                                            offense - protection);
    return static_cast<int>(std::min<std::int64_t>(target.hp, raw));
}

}  // namespace

Battle::Battle(Actor player, Actor enemy, DamageRules rules)
    : player_(std::move(player)), enemy_(std::move(enemy)), rules_(rules) {
    validate_actor(player_);
    validate_actor(enemy_);
    validate_rules(rules_);
    if (player_.id == enemy_.id) {
        throw std::invalid_argument("each combatant must have a distinct ID");
    }
}

bool Battle::finished() const noexcept {
    return player_.hp == 0 || enemy_.hp == 0;
}

std::optional<Side> Battle::winner() const noexcept {
    if (enemy_.hp == 0) return Side::player;
    if (player_.hp == 0) return Side::enemy;
    return std::nullopt;
}

std::optional<Side> Battle::next_turn() const noexcept {
    if (finished()) return std::nullopt;
    return turn_;
}

AttackEvent Battle::attack(Side side) {
    if (finished()) throw std::logic_error("battle already finished");
    if (turn_ != side) throw std::logic_error("action taken out of turn");

    Actor& source = side == Side::player ? player_ : enemy_;
    Actor& target = side == Side::player ? enemy_ : player_;
    const int before = target.hp;
    const int damage = damage_for(source, target, rules_);
    target.hp -= damage;
    const bool ended = finished();
    if (!ended) turn_ = side == Side::player ? Side::enemy : Side::player;
    return {side, source.id, target.id, damage, before, target.hp, ended};
}

}  // namespace modernps
