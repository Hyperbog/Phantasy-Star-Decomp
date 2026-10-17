#pragma once

#include "modernps/story.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace modernps {

struct DialogueLine {
    std::string requires_flag;  // empty means fallback text
    std::string text;
};

struct NpcDefinition {
    std::string id;
    std::string name;
    std::vector<DialogueLine> dialogue;
};

struct DialogueResult {
    std::string npc_id;
    std::string text;
    std::string matched_flag;
};

class DialogueEngine {
public:
    void add_npc(NpcDefinition npc);
    void bind(const WorldMap& map, Position position, std::string_view npc_id);
    DialogueResult talk(std::string_view npc_id, const EventEngine& events) const;
    std::optional<DialogueResult> talk_here(const WorldState& world,
                                             const EventEngine& events) const;
private:
    std::map<std::string, NpcDefinition, std::less<>> npcs_;
    std::map<std::tuple<std::string, int, int>, std::string> locations_;
};

} // namespace modernps
