#include "modernps/content.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace modernps;
namespace {
void require(bool yes, const char* message) { if (!yes) throw std::runtime_error(message); }
template<class F> void invalid(F&& fn) {
    bool caught = false;
    try { fn(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, "malformed content was accepted");
}
constexpr auto valid_item = "PSITEM 1\nid=tonic\nname=Tonic\nkind=consumable\nmax_stack=5\nhealing=8\nattack_bonus=0\ndefense_bonus=0\nspeed_bonus=0\n";
constexpr auto valid_event = "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=0\naction_count=2\naction=give_item:tonic:1\naction=set_flag:found:1\n";
constexpr auto valid_quest = "PSQUEST 1\nid=quest\ntitle=A quest\nactivation_flag=found\ncompletion_flag=done\n";
void valid_documents() {
    std::istringstream a(valid_item), b(valid_event), c(valid_quest);
    require(load_item_definition(a).healing == 8, "item parser failed");
    require(load_event_definition(b).actions.size() == 2, "event parser failed");
    require(load_quest_definition(c).completion_flag == "done", "quest parser failed");
}
void sample_documents() {
    const std::string base = MODERNPS_CONTENT_DIR;
    for (const auto& [name, type] : std::vector<std::pair<std::string, int>>{
             {"training_tonic.psitem", 0}, {"training_blade.psitem", 0},
             {"training_cache.psevent", 1}, {"training_gate.psevent", 1},
             {"training_quest.psquest", 2}}) {
        std::ifstream file(base + "/" + name);
        require(bool(file), "example file missing");
        if (type == 0) { ItemCatalog catalog; catalog.add(load_item_definition(file)); }
        if (type == 1) { EventEngine engine; engine.add_event(load_event_definition(file)); }
        if (type == 2) { QuestJournal journal; journal.add_quest(load_quest_definition(file)); }
    }
}
void item_fail_closed() {
    const std::vector<std::string> invalid_docs = {
        "PSITEM 2\nid=tonic\n", "PSITEM 1\nid=tonic\n",
        std::string(valid_item) + "extra=1\n",
        "PSITEM 1\nid=tonic\nname=Tonic\nkind=unknown\nmax_stack=5\nhealing=8\nattack_bonus=0\ndefense_bonus=0\nspeed_bonus=0\n",
        "PSITEM 1\nid=tonic\nname=Tonic\nkind=consumable\nmax_stack=5\nhealing=0\nattack_bonus=0\ndefense_bonus=0\nspeed_bonus=0\n",
        "PSITEM 1\nid=tonic\nname=Tonic\nkind=consumable\nmax_stack=5\nhealing=8\nattack_bonus=1\ndefense_bonus=0\nspeed_bonus=0\n",
        "PSITEM 1\nid=tonic\nname=Tonic\nkind=consumable\nmax_stack=9999999999999999\nhealing=8\nattack_bonus=0\ndefense_bonus=0\nspeed_bonus=0\n"
    };
    for (const auto& source : invalid_docs) {
        std::istringstream doc(source);
        invalid([&] { (void)load_item_definition(doc); });
    }
}
void event_fail_closed() {
    const std::vector<std::string> invalid_docs = {
        "PSEVENT 2\nid=pickup\n",
        std::string(valid_event) + "trailer=1\n",
        "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=0\naction_count=1\naction=give_item:tonic\n",
        "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=0\naction_count=1\naction=give_item:tonic:1:extra\n",
        "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=0\naction_count=1\naction=unknown:tonic:1\n",
        "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=0\naction_count=1\naction=set_flag:found:2\n",
        "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=0\naction_count=1\naction=give_item:tonic:-1\n",
        "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=0\naction_count=2\naction=give_item:tonic:1\n",
        "PSEVENT 1\nid=pickup\nrequires_flag=\nrepeatable=3\naction_count=1\naction=give_item:tonic:1\n",
    };
    for (const auto& source : invalid_docs) {
        std::istringstream doc(source);
        invalid([&] { (void)load_event_definition(doc); });
    }
}
void quest_fail_closed() {
    const std::vector<std::string> invalid_docs = {
        "PSQUEST 2\nid=quest\n",
        "PSQUEST 1\nid=quest\ntitle=A quest\nactivation_flag=found\ncompletion_flag=found\n",
        "PSQUEST 1\nid=quest\ntitle=\nactivation_flag=found\ncompletion_flag=done\n",
        std::string(valid_quest) + "trailer=1\n"
    };
    for (const auto& source : invalid_docs) {
        std::istringstream doc(source);
        invalid([&] { (void)load_quest_definition(doc); });
    }
}
void crlf_and_strict_trailer() {
    std::string crlf;
    for (const char c : std::string(valid_item)) {
        if (c == '\n') crlf += "\r\n"; else crlf += c;
    }
    std::istringstream document(crlf);
    require(load_item_definition(document).id == "tonic", "CRLF unsupported");
    std::istringstream trailer(std::string(valid_quest) + "\n");
    invalid([&] { (void)load_quest_definition(trailer); });
}
}
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"valid_documents",valid_documents}, {"sample_documents",sample_documents},
        {"item_fail_closed",item_fail_closed}, {"event_fail_closed",event_fail_closed},
        {"quest_fail_closed",quest_fail_closed}, {"crlf_and_strict_trailer",crlf_and_strict_trailer}
    };
    try {
        for (const auto& [name, fn] : tests) { fn(); std::cout << "PASS " << name << '\n'; }
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
    return 0;
}
