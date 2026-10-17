#include "modernps/character.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace modernps {
namespace {
constexpr int max_stat = 1'000'000;

void validate_stat(int start, int growth, bool must_be_positive) {
    const std::int64_t cap = static_cast<std::int64_t>(start) +
                             static_cast<std::int64_t>(Character::max_level - 1) * growth;
    if (start < (must_be_positive ? 1 : 0) || growth < 0 || cap > max_stat) {
        throw std::invalid_argument("invalid base stat or growth at level cap");
    }
}
}

Character::Character(CharacterDefinition definition)
    : definition_(std::move(definition)), stats_(definition_.base),
      hp_(definition_.base.max_hp) {
    if (definition_.id.empty() || definition_.display_name.empty()) {
        throw std::invalid_argument("character requires id and display name");
    }
    validate_stat(stats_.max_hp, definition_.growth.max_hp, true);
    validate_stat(stats_.attack, definition_.growth.attack, false);
    validate_stat(stats_.defense, definition_.growth.defense, false);
    validate_stat(stats_.speed, definition_.growth.speed, false);
}

int Character::required_experience_for_next_level(int current_level) noexcept {
    return 10 * current_level * current_level; // cumulative threshold
}

int Character::grant_experience(int amount) {
    if (amount < 0 || static_cast<std::int64_t>(experience_) + amount > max_experience) {
        throw std::invalid_argument("negative or overflowing experience gain");
    }
    experience_ += amount;
    const int prior_level = level_;
    while (level_ < max_level &&
           experience_ >= required_experience_for_next_level(level_)) {
        ++level_;
    }
    const int increments = level_ - prior_level;
    if (increments != 0) {
        stats_.max_hp += increments * definition_.growth.max_hp;
        stats_.attack += increments * definition_.growth.attack;
        stats_.defense += increments * definition_.growth.defense;
        stats_.speed += increments * definition_.growth.speed;
        hp_ += increments * definition_.growth.max_hp;
    }
    return increments;
}

Actor Character::as_combat_actor() const {
    return {definition_.id, stats_, hp_};
}

Party::Party(Character leader) {
    members_.push_back(std::move(leader));
}

bool Party::contains(std::string_view id) const noexcept {
    return std::any_of(members_.begin(), members_.end(),
                       [id](const Character& actor) {
                           return actor.definition().id == id;
                       });
}

void Party::add(Character member) {
    if (members_.size() >= max_members || contains(member.definition().id)) {
        throw std::invalid_argument("party full or member id duplicated");
    }
    members_.push_back(std::move(member));
}

Character& Party::member(std::string_view id) {
    const auto it = std::find_if(members_.begin(), members_.end(),
                                 [id](const Character& actor) {
                                     return actor.definition().id == id;
                                 });
    if (it == members_.end()) throw std::out_of_range("character not in party");
    return *it;
}

const Character& Party::member(std::string_view id) const {
    const auto it = std::find_if(members_.cbegin(), members_.cend(),
                                 [id](const Character& actor) {
                                     return actor.definition().id == id;
                                 });
    if (it == members_.cend()) throw std::out_of_range("character not in party");
    return *it;
}

} // namespace modernps
