#include "modernps/save.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace modernps;
namespace {
void check(bool good, const char* msg) { if (!good) throw std::runtime_error(msg); }
template <typename Error, typename Fn> void rejects(Fn&& fn) {
    bool threw = false;
    try { fn(); } catch (const Error&) { threw = true; }
    check(threw, "invalid save was accepted");
}
ItemCatalog make_catalog() {
    ItemCatalog items;
    items.add({"tonic", "Tonic", ItemKind::consumable, 5, 8, 0, 0, 0});
    items.add({"blade", "Blade", ItemKind::weapon, 1, 0, 2, 0, 0});
    return items;
}
GameState blueprint() {
    WorldState world(WorldMap("training", 4, 3, {"....", ".#..", "...."}, {0,0}));
    Party party(Character({"hero", "Hero", {30, 9, 4, 3}, {4, 2, 1, 1}}));
    EventEngine events;
    events.add_event({"reward", "", false, {{EventActionKind::set_flag, "quest_done", 1}}});
    return {world, party, Inventory(3), {}, events, Wallet(0)};
}
GameState progressed(const ItemCatalog& items) {
    auto state = blueprint();
    state.world.move(Direction::east);
    state.world.move(Direction::east);
    state.party.member("hero").grant_experience(40);
    state.party.member("hero").take_damage(7);
    state.inventory.add(items, "tonic", 3);
    state.inventory.add(items, "blade", 1);
    Loadout loadout;
    loadout.equip(items, state.inventory, "blade");
    state.equipment.emplace("hero", loadout);
    state.wallet.credit(178);
    if (state.events.interact("reward", state.party, state.inventory, items) != EventResult::applied)
        throw std::runtime_error("test fixture quest failed");
    return state;
}
std::string repair_checksum(std::string bytes) {
    const auto marker = bytes.rfind("checksum=");
    if (marker == std::string::npos) throw std::runtime_error("bad test save template");
    const auto payload = bytes.substr(0, marker);
    std::uint32_t crc = 0xffffffffu;
    for (unsigned char c : payload) {
        crc ^= c;
        for (int j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    }
    crc ^= 0xffffffffu;
    std::ostringstream formatted;
    formatted << "checksum=" << std::hex << std::setfill('0') << std::setw(8) << crc << '\n';
    return payload + formatted.str();
}
std::string replace_record(const std::string& original, std::string_view from,
                           std::string_view to) {
    auto save = original;
    const auto found = save.find(from);
    check(found != std::string::npos, "missing fixture record");
    save.replace(found, from.size(), to);
    return repair_checksum(save);
}
void complete_roundtrip() {
    auto items = make_catalog();
    auto before = progressed(items);
    const auto encoded = encode_save(before);
    const auto loaded = decode_save(encoded, blueprint(), items);
    check(encode_save(loaded) == encoded, "roundtrip changed serialized game state");
    check(loaded.world.position() == Position{2,0} &&
          loaded.world.facing() == Direction::east, "world not restored");
    check(loaded.party.member("hero").level() == 3 &&
          loaded.party.member("hero").hp() == 31, "character progress not restored");
    check(loaded.inventory.count("tonic") == 3 && loaded.wallet.balance() == 178,
          "inventory/wallet not restored");
    check(loaded.equipment.at("hero").equipped(ItemKind::weapon) == "blade" &&
          loaded.equipment.at("hero").effective_stats(loaded.party.member("hero"), items).attack == 15,
          "equipment not restored");
    check(loaded.events.has_flag("quest_done") && loaded.events.completed("reward"),
          "story flags not restored");
}
void corruption_and_version_fail_closed() {
    auto items = make_catalog();
    const auto valid = encode_save(progressed(items));
    auto changed = valid;
    changed[20] = changed[20] == 'a' ? 'b' : 'a';
    rejects<std::invalid_argument>([&] { (void)decode_save(changed, blueprint(), items); });
    rejects<std::invalid_argument>([&] { (void)decode_save(valid.substr(0,valid.size()-2), blueprint(), items); });
    rejects<std::invalid_argument>([&] { (void)decode_save(valid + "junk", blueprint(), items); });
    rejects<std::invalid_argument>([&] { (void)decode_save(replace_record(valid,"PSAVE 1", "PSAVE 2"), blueprint(), items); });
    rejects<std::invalid_argument>([&] { (void)decode_save(std::string(129*1024,'x'), blueprint(), items); });
}
void semantic_invalidity_with_correct_checksum() {
    auto items = make_catalog();
    const auto valid = encode_save(progressed(items));
    for (const auto& [from, to] : {
            std::pair{"map=training", "map=other___"},
            std::pair{"x=2", "x=20"},
            std::pair{"y=0", "y=6"},
            std::pair{"wallet=178", "wallet=-1"},
            std::pair{"actor=hero,40,31", "actor=hero,40,900"},
            std::pair{"item=tonic,3", "item=tonic,99"},
            std::pair{"loadout=hero,blade,-,-", "loadout=hero,tonic,-,-"},
            std::pair{"event=reward", "event=unknown"},
            std::pair{"flag=quest_done", "flag=bad-name"},
        }) {
        const auto forged = replace_record(valid, from, to);
        rejects<std::invalid_argument>([&] { (void)decode_save(forged, blueprint(), items); });
    }
}
void incompatible_content_rejected() {
    auto items = make_catalog();
    const auto valid = encode_save(progressed(items));
    auto template_state = blueprint();
    template_state.party.member("hero").grant_experience(10);
    // An already-progressed blueprint should still restore original definitions.
    check(encode_save(decode_save(valid, template_state, items)) == valid,
          "restored character used prior blueprint progression");
    ItemCatalog incomplete;
    incomplete.add({"tonic", "Tonic", ItemKind::consumable, 5, 8, 0, 0, 0});
    rejects<std::out_of_range>([&] { (void)decode_save(valid, blueprint(), incomplete); });
    auto wrong_events = blueprint();
    wrong_events.events = EventEngine{};
    rejects<std::invalid_argument>([&] { (void)decode_save(valid, wrong_events, items); });
}
void dead_party_member_restored() {
    auto items = make_catalog();
    auto state = blueprint();
    state.party.member("hero").take_damage(30);
    const auto loaded = decode_save(encode_save(state), blueprint(), items);
    check(loaded.party.member("hero").hp() == 0, "zero-HP member not restored");
}
void disk_roundtrip_and_failed_overwrite() {
    auto items = make_catalog();
    auto state = progressed(items);
    const auto filename = "modernps_test_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()) + ".psave";
    const auto path = std::filesystem::temp_directory_path() / filename;
    const auto temp_path = std::filesystem::path(path.string() + ".tmp");
    try {
        write_save_file(path, state);
        check(encode_save(read_save_file(path, blueprint(), items)) == encode_save(state),
              "disk persistence did not restore state");
        {
            std::ofstream pending(temp_path);
            pending << "do not replace";
        }
        rejects<std::runtime_error>([&] { write_save_file(path, state); });
        check(encode_save(read_save_file(path, blueprint(), items)) == encode_save(state),
              "pre-existing temp file affected committed save");
        std::filesystem::remove(temp_path);
        {
            std::ofstream corrupt(path, std::ios::binary|std::ios::trunc);
            corrupt << "PSAVE 1\ntruncated";
        }
        rejects<std::invalid_argument>([&] { (void)read_save_file(path, blueprint(), items); });
    } catch (...) {
        std::filesystem::remove(path);
        std::filesystem::remove(temp_path);
        throw;
    }
    std::filesystem::remove(path);
}
} // namespace
int main() {
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"complete_roundtrip", complete_roundtrip},
        {"corruption_and_version_fail_closed", corruption_and_version_fail_closed},
        {"semantic_invalidity_with_correct_checksum", semantic_invalidity_with_correct_checksum},
        {"incompatible_content_rejected", incompatible_content_rejected},
        {"dead_party_member_restored", dead_party_member_restored},
        {"disk_roundtrip_and_failed_overwrite", disk_roundtrip_and_failed_overwrite},
    };
    try { for (auto [name,fn]:tests) { fn(); std::cout << "PASS " << name << '\n'; } }
    catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
    return 0;
}
