#include "modernps/story.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace modernps {

void EventEngine::add_event(EventDefinition definition) {
    if (!valid_content_id(definition.id) ||
        (!definition.requires_flag.empty() && !valid_content_id(definition.requires_flag)) ||
        definition.actions.empty() || definition.actions.size() > 16) {
        throw std::invalid_argument("invalid event definition");
    }
    for (const auto& action : definition.actions) {
        if (!valid_content_id(action.target) || action.amount < 1 ||
            action.amount > 1'000'000) {
            throw std::invalid_argument("invalid event action");
        }
        switch (action.kind) {
            case EventActionKind::set_flag:
                if (action.amount != 1) throw std::invalid_argument("flags have no quantity");
                break;
            case EventActionKind::give_item:
            case EventActionKind::grant_experience:
                break;
            default: throw std::invalid_argument("unknown event action");
        }
    }
    const auto id = definition.id;
    if (!definitions_.emplace(id, std::move(definition)).second)
        throw std::invalid_argument("duplicate event id");
}

bool EventEngine::contains(std::string_view id) const noexcept {
    return definitions_.find(id) != definitions_.end();
}

bool EventEngine::has_flag(std::string_view flag) const noexcept {
    return flags_.find(flag) != flags_.end();
}

bool EventEngine::completed(std::string_view id) const noexcept {
    return completed_.find(id) != completed_.end();
}

EventResult EventEngine::interact(std::string_view id, Party& party,
                                  Inventory& inventory, const ItemCatalog& catalog) {
    const auto found = definitions_.find(id);
    if (found == definitions_.end()) throw std::out_of_range("unknown event");
    const auto& event = found->second;
    if (!event.repeatable && completed(id)) return EventResult::already_completed;
    if (!event.requires_flag.empty() && !has_flag(event.requires_flag))
        return EventResult::locked;

    // Stage *all* state before applying actions. Even a later invalid action,
    // full inventory or experience overflow cannot commit a partial reward.
    Party staged_party = party;
    Inventory staged_inventory = inventory;
    auto staged_flags = flags_;
    auto staged_completed = completed_;
    for (const auto& action : event.actions) {
        switch (action.kind) {
            case EventActionKind::set_flag:
                staged_flags.insert(action.target);
                break;
            case EventActionKind::give_item:
                staged_inventory.add(catalog, action.target, action.amount);
                break;
            case EventActionKind::grant_experience:
                staged_party.member(action.target).grant_experience(action.amount);
                break;
        }
    }
    staged_completed.insert(event.id);
    party.swap(staged_party);
    inventory.swap(staged_inventory);
    flags_.swap(staged_flags);
    completed_.swap(staged_completed);
    return EventResult::applied;
}

void QuestJournal::add_quest(QuestDefinition definition) {
    if (!valid_content_id(definition.id) || definition.title.empty() ||
        definition.title.size() > 80 ||
        std::any_of(definition.title.begin(), definition.title.end(), [](unsigned char c) {
            return c < 0x20 || c == 0x7f;
        }) || !valid_content_id(definition.activation_flag) ||
        !valid_content_id(definition.completion_flag) ||
        definition.activation_flag == definition.completion_flag) {
        throw std::invalid_argument("invalid quest definition");
    }
    const auto id = definition.id;
    if (!quests_.emplace(id, std::move(definition)).second)
        throw std::invalid_argument("duplicate quest id");
}

QuestStatus QuestJournal::status(std::string_view id, const EventEngine& events) const {
    const auto found = quests_.find(id);
    if (found == quests_.end()) throw std::out_of_range("unknown quest");
    const auto& quest = found->second;
    if (events.has_flag(quest.completion_flag)) return QuestStatus::completed;
    if (events.has_flag(quest.activation_flag)) return QuestStatus::active;
    return QuestStatus::unavailable;
}

void WorldInteractions::bind(const WorldMap& map, Position pos,
                             std::string_view event_id, const EventEngine& events) {
    if (!map.walkable(pos) || !events.contains(event_id))
        throw std::invalid_argument("invalid world event binding");
    const auto key = std::make_tuple(map.id(), pos.x, pos.y);
    if (!locations_.emplace(key, std::string(event_id)).second)
        throw std::invalid_argument("world tile already has an event");
}

std::optional<EventResult> WorldInteractions::interact_here(
    const WorldState& world, EventEngine& events, Party& party,
    Inventory& inventory, const ItemCatalog& catalog) const {
    const auto pos = world.position();
    const auto found = locations_.find(std::make_tuple(world.map().id(), pos.x, pos.y));
    if (found == locations_.end()) return std::nullopt;
    return events.interact(found->second, party, inventory, catalog);
}

} // namespace modernps
