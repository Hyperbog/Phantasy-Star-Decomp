#include "modernps/story.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace modernps;
namespace {
void require(bool yes, const char* msg) { if (!yes) throw std::runtime_error(msg); }
template<class E, class F> void rejects(F&& fn) {
    bool caught = false;
    try { fn(); } catch (const E&) { caught = true; }
    require(caught, "expected rejection");
}
Character hero() { return Character({"hero", "Hero", {30, 9, 4, 5}, {1, 1, 1, 1}}); }
ItemCatalog items() {
    ItemCatalog c;
    c.add({"tonic", "Tonic", ItemKind::consumable, 3, 6, 0, 0, 0});
    c.add({"seal", "Seal", ItemKind::key_item, 1, 0, 0, 0, 0});
    return c;
}
void event_and_journal() {
    EventEngine e; QuestJournal quests; auto c = items();
    Party party(hero()); Inventory inv;
    quests.add_quest({"test", "A Test", "started", "finished"});
    require(quests.status("test", e) == QuestStatus::unavailable, "quest initial state");
    e.add_event({"begin", "", false, {{EventActionKind::set_flag, "started", 1},
                                       {EventActionKind::give_item, "tonic", 2}}});
    e.add_event({"finish", "started", false, {{EventActionKind::grant_experience, "hero", 10},
                                               {EventActionKind::set_flag, "finished", 1}}});
    require(e.interact("finish", party, inv, c) == EventResult::locked, "prerequisite ignored");
    require(e.interact("begin", party, inv, c) == EventResult::applied, "start not applied");
    require(e.has_flag("started") && inv.count("tonic") == 2 &&
            quests.status("test", e) == QuestStatus::active, "quest not activated");
    require(e.interact("begin", party, inv, c) == EventResult::already_completed,
            "non-repeatable event ran twice");
    require(e.interact("finish", party, inv, c) == EventResult::applied,
            "quest finish failed");
    require(quests.status("test", e) == QuestStatus::completed &&
            party.member("hero").level() == 2 && party.member("hero").experience() == 10,
            "quest completion or reward wrong");
}
void reward_rollback_invalid_last_action() {
    auto c = items(); Party party(hero()); Inventory inv;
    EventEngine events;
    events.add_event({"trap", "", false, {{EventActionKind::give_item, "tonic", 1},
                                         {EventActionKind::set_flag, "rollback_flag", 1},
                                         {EventActionKind::grant_experience, "unknown", 10}}});
    rejects<std::out_of_range>([&] { events.interact("trap", party, inv, c); });
    require(inv.count("tonic") == 0 && !events.has_flag("rollback_flag") &&
            !events.completed("trap") && party.member("hero").experience() == 0,
            "multi-action partial commit");
}
void reward_rollback_full_inventory() {
    auto c = items(); Party party(hero()); Inventory inv(1);
    inv.add(c, "seal", 1); EventEngine events;
    events.add_event({"trap", "", false, {{EventActionKind::grant_experience, "hero", 10},
                                         {EventActionKind::give_item, "tonic", 1}}});
    rejects<std::invalid_argument>([&] { events.interact("trap", party, inv, c); });
    require(party.member("hero").experience() == 0 && inv.count("seal") == 1 &&
            !events.completed("trap"), "inventory failure partially rewarded XP");
}
void repeatable_events_and_caps() {
    auto c = items(); Party party(hero()); Inventory inv;
    EventEngine events;
    events.add_event({"fountain", "", true, {{EventActionKind::give_item, "tonic", 1}}});
    for (int n = 0; n < 3; ++n)
        require(events.interact("fountain", party, inv, c) == EventResult::applied,
                "repeatable action rejected");
    rejects<std::invalid_argument>([&] { events.interact("fountain", party, inv, c); });
    require(inv.count("tonic") == 3 && events.completed("fountain"),
            "repeatable cap violation changed inventory");
}
void invalid_event_and_quest_data() {
    EventEngine e; QuestJournal quests;
    rejects<std::invalid_argument>([&] { e.add_event({"", "", false, {{EventActionKind::set_flag, "yes", 1}}}); });
    rejects<std::invalid_argument>([&] { e.add_event({"no", "", false, {}}); });
    rejects<std::invalid_argument>([&] { e.add_event({"no", "", false, {{EventActionKind::set_flag, "yes", 2}}}); });
    rejects<std::invalid_argument>([&] { e.add_event({"no", "", false, {{static_cast<EventActionKind>(99), "yes", 1}}}); });
    e.add_event({"valid", "", false, {{EventActionKind::set_flag, "ready", 1}}});
    rejects<std::invalid_argument>([&] { e.add_event({"valid", "", true, {{EventActionKind::set_flag, "ready", 1}}}); });
    rejects<std::invalid_argument>([&] { quests.add_quest({"quest", "Q", "same", "same"}); });
    quests.add_quest({"quest", "Q", "one", "two"});
    rejects<std::invalid_argument>([&] { quests.add_quest({"quest", "Q", "one", "two"}); });
    rejects<std::out_of_range>([&] { quests.status("absent", e); });
}
void world_binding() {
    WorldMap map("room", 4, 3, {"####", "#..#", "####"}, {1, 1});
    WorldState world(map);
    EventEngine e; e.add_event({"pickup", "", false, {{EventActionKind::give_item, "tonic", 1}}});
    WorldInteractions bindings;
    auto c = items(); Party party(hero()); Inventory inv;
    bindings.bind(map, {2, 1}, "pickup", e);
    require(!bindings.interact_here(world, e, party, inv, c), "unexpected event on spawn");
    rejects<std::invalid_argument>([&] { bindings.bind(map, {0, 1}, "pickup", e); });
    rejects<std::invalid_argument>([&] { bindings.bind(map, {1, 1}, "absent", e); });
    rejects<std::invalid_argument>([&] { bindings.bind(map, {2, 1}, "pickup", e); });
    world.move(Direction::east);
    require(bindings.interact_here(world, e, party, inv, c) == EventResult::applied &&
            inv.count("tonic") == 1, "tile event did not trigger");
    require(bindings.interact_here(world, e, party, inv, c) == EventResult::already_completed,
            "one-shot tile did not remain completed");
}
void missing_event_is_no_op() {
    EventEngine e; auto c = items(); Party party(hero()); Inventory inv;
    rejects<std::out_of_range>([&] { e.interact("unknown", party, inv, c); });
    require(party.members().size() == 1 && inv.slots_used() == 0, "missing event mutated state");
}
}
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"event_and_journal", event_and_journal},
        {"reward_rollback_invalid_last_action", reward_rollback_invalid_last_action},
        {"reward_rollback_full_inventory", reward_rollback_full_inventory},
        {"repeatable_events_and_caps", repeatable_events_and_caps},
        {"invalid_event_and_quest_data", invalid_event_and_quest_data},
        {"world_binding", world_binding}, {"missing_event_is_no_op", missing_event_is_no_op},
    };
    try {
        for (auto [name, fn] : tests) { fn(); std::cout << "PASS " << name << '\n'; }
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
    return 0;
}
