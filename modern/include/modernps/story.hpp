#pragma once

#include "modernps/items.hpp"
#include "modernps/world.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace modernps {

enum class EventActionKind { set_flag, give_item, grant_experience };
struct EventAction {
    EventActionKind kind;
    std::string target;
    int amount;
};
struct EventDefinition {
    std::string id;
    std::string requires_flag; // empty = no prerequisite
    bool repeatable;
    std::vector<EventAction> actions;
};

enum class EventResult { applied, locked, already_completed };

// Pure domain-state machine. All action effects commit together, or none do.
class EventEngine {
public:
    void add_event(EventDefinition definition);
    bool contains(std::string_view id) const noexcept;
    bool has_flag(std::string_view flag) const noexcept;
    bool completed(std::string_view id) const noexcept;
    const std::set<std::string, std::less<>>& flags() const noexcept { return flags_; }
    const std::set<std::string, std::less<>>& completed_events() const noexcept { return completed_; }
    void restore_progress(const std::set<std::string, std::less<>>& flags,
                          const std::set<std::string, std::less<>>& completed);
    EventResult interact(std::string_view id, Party& party, Inventory& inventory,
                         const ItemCatalog& catalog);
private:
    std::map<std::string, EventDefinition, std::less<>> definitions_;
    std::set<std::string, std::less<>> flags_;
    std::set<std::string, std::less<>> completed_;
};

struct QuestDefinition {
    std::string id;
    std::string title;
    std::string activation_flag;
    std::string completion_flag;
};
enum class QuestStatus { unavailable, active, completed };

class QuestJournal {
public:
    void add_quest(QuestDefinition definition);
    QuestStatus status(std::string_view id, const EventEngine& events) const;
private:
    std::map<std::string, QuestDefinition, std::less<>> quests_;
};

// Binds explicit map coordinates to event IDs; no proximity guesswork.
class WorldInteractions {
public:
    void bind(const WorldMap& map, Position position,
              std::string_view event_id, const EventEngine& events);
    std::optional<EventResult> interact_here(const WorldState& world,
                                             EventEngine& events, Party& party,
                                             Inventory& inventory,
                                             const ItemCatalog& catalog) const;
private:
    std::map<std::tuple<std::string, int, int>, std::string> locations_;
};

} // namespace modernps
