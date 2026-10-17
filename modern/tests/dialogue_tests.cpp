#include "modernps/dialogue.hpp"
#include "modernps/content.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace modernps;
namespace {
void check(bool good, const char* msg) { if (!good) throw std::runtime_error(msg); }
template <typename Error, typename Fn> void rejects(Fn&& fn) {
    bool threw = false;
    try { fn(); } catch (const Error&) { threw = true; }
    check(threw, "expected exception missing");
}
void conditional_dialogue() {
    DialogueEngine engine;
    engine.add_npc({"scholar", "Scholar", {{"", "Before"}, {"quest_done", "After"}}});
    EventEngine events;
    check(engine.talk("scholar", events).text == "Before", "fallback wrong");
    events.add_event({"resolve", "", false, {{EventActionKind::set_flag, "quest_done", 1}}});
    ItemCatalog items; Party party(Character({"hero", "Hero", {20, 3, 3, 3}, {1, 1, 1, 1}}));
    Inventory inventory;
    check(events.interact("resolve", party, inventory, items) == EventResult::applied,
          "flag event failed");
    auto result = engine.talk("scholar", events);
    check(result.text == "After" && result.matched_flag == "quest_done", "conditional response wrong");
}
void coordinate_binding() {
    WorldMap map("room", 3, 2, {"...", ".#."}, {0,0});
    WorldState state(map);
    DialogueEngine engine;
    engine.add_npc({"guide", "Guide", {{"", "Hello"}}});
    engine.bind(map, {1,0}, "guide");
    EventEngine events;
    check(!engine.talk_here(state, events), "NPC spoke from wrong position");
    state.move(Direction::east);
    check(engine.talk_here(state, events)->text == "Hello", "NPC missing at location");
    rejects<std::invalid_argument>([&] { engine.bind(map, {1,1}, "guide"); });
    rejects<std::invalid_argument>([&] { engine.bind(map, {1,0}, "guide"); });
    rejects<std::out_of_range>([&] { engine.talk("absent", events); });
}
void malformed_dialogue_rejected() {
    DialogueEngine engine;
    rejects<std::invalid_argument>([&] { engine.add_npc({"merchant", "Merchant", {{"progress", "No fallback"}}}); });
    rejects<std::invalid_argument>([&] { engine.add_npc({"merchant", "Merchant", {{"", "Contains\nnewline"}}}); });
    rejects<std::invalid_argument>([&] { engine.add_npc({"merchant", "Merchant", {}}); });
}
void strict_content() {
    std::ifstream file(std::string(MODERNPS_CONTENT_DIR) + "/training_merchant.psnpc");
    check(static_cast<bool>(file), "missing NPC content");
    auto npc = load_npc_definition(file);
    check(npc.id == "training_merchant" && npc.dialogue.size() == 2, "NPC content wrong");
    std::istringstream bad("PSNPC 2\nid=merchant\nname=Merchant\nlines=1\nline=-|Hi\n");
    rejects<std::invalid_argument>([&] { (void)load_npc_definition(bad); });
    std::istringstream missing("PSNPC 1\nid=merchant\nname=Merchant\nlines=2\nline=-|Hi\n");
    rejects<std::invalid_argument>([&] { (void)load_npc_definition(missing); });
    std::istringstream duplicate("PSNPC 1\nid=merchant\nname=Merchant\nlines=1\nline=-|Hi\nextra=1\n");
    rejects<std::invalid_argument>([&] { (void)load_npc_definition(duplicate); });
}
void strict_shop_content() {
    ItemCatalog items;
    std::ifstream tonic(std::string(MODERNPS_CONTENT_DIR)+"/training_tonic.psitem");
    std::ifstream blade(std::string(MODERNPS_CONTENT_DIR)+"/training_blade.psitem");
    items.add(load_item_definition(tonic)); items.add(load_item_definition(blade));
    std::ifstream shop_file(std::string(MODERNPS_CONTENT_DIR)+"/training_shop.psshop");
    check(static_cast<bool>(shop_file), "missing shop content");
    auto def = load_shop_definition(shop_file, items);
    Shop shop(def, items);
    check(shop.offer("training_tonic").buy_price == 12, "shop content incorrect");
    for (const std::string invalid : {
            "PSSHOP 2\nid=market\noffers=1\noffer=training_tonic,12,6\n",
            "PSSHOP 1\nid=market\noffers=1\noffer=unknown,12,6\n",
            "PSSHOP 1\nid=market\noffers=1\noffer=training_tonic,12,13\n",
            "PSSHOP 1\nid=market\noffers=1\noffer=training_tonic,12,6\nextra=1\n"}) {
        std::istringstream input(invalid);
        rejects<std::invalid_argument>([&] { (void)load_shop_definition(input, items); });
    }
}
} // namespace
int main() {
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"conditional_dialogue", conditional_dialogue},
        {"coordinate_binding", coordinate_binding},
        {"malformed_dialogue_rejected", malformed_dialogue_rejected},
        {"strict_content", strict_content},
        {"strict_shop_content", strict_shop_content},
    };
    try { for (auto [name,fn]:tests) { fn(); std::cout << "PASS " << name << '\n'; } }
    catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
    return 0;
}
