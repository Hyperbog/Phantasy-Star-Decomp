#include "modernps/dialogue.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace modernps {
namespace {
bool printable(const std::string& text, std::size_t limit) {
    return !text.empty() && text.size() <= limit &&
           std::none_of(text.begin(), text.end(), [](unsigned char c) {
               return c < 0x20 || c == 0x7f;
           });
}
} // namespace

void DialogueEngine::add_npc(NpcDefinition npc) {
    if (!valid_content_id(npc.id) || !printable(npc.name, 80) ||
        npc.dialogue.empty() || npc.dialogue.size() > 16 ||
        !npc.dialogue.front().requires_flag.empty())
        throw std::invalid_argument("invalid NPC identity or missing fallback line");
    for (const auto& line : npc.dialogue) {
        if ((!line.requires_flag.empty() && !valid_content_id(line.requires_flag)) ||
            !printable(line.text, 160))
            throw std::invalid_argument("invalid NPC dialogue line");
    }
    auto id = npc.id;
    if (!npcs_.emplace(id, std::move(npc)).second)
        throw std::invalid_argument("duplicate NPC id");
}

void DialogueEngine::bind(const WorldMap& map, Position pos, std::string_view id) {
    if (!map.walkable(pos) || npcs_.find(id) == npcs_.end())
        throw std::invalid_argument("invalid NPC location or id");
    if (!locations_.emplace(std::make_tuple(map.id(), pos.x, pos.y), std::string(id)).second)
        throw std::invalid_argument("NPC already at location");
}

DialogueResult DialogueEngine::talk(std::string_view id, const EventEngine& events) const {
    const auto found = npcs_.find(id);
    if (found == npcs_.end()) throw std::out_of_range("unknown NPC");
    const DialogueLine* chosen = &found->second.dialogue.front();
    // Later lines can override a fallback; highest-priority satisfied line wins.
    for (const auto& line : found->second.dialogue) {
        if (!line.requires_flag.empty() && events.has_flag(line.requires_flag))
            chosen = &line;
    }
    return {found->first, chosen->text, chosen->requires_flag};
}

std::optional<DialogueResult> DialogueEngine::talk_here(
    const WorldState& world, const EventEngine& events) const {
    const auto pos = world.position();
    auto found = locations_.find(std::make_tuple(world.map().id(), pos.x, pos.y));
    if (found == locations_.end()) return std::nullopt;
    return talk(found->second, events);
}

} // namespace modernps
