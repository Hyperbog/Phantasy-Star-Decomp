#pragma once

#include "modernps/character.hpp"
#include "modernps/world.hpp"
#include "modernps/story.hpp"

#include <iosfwd>

namespace modernps {

// Strict, human-editable, versioned v1 text formats; no third-party parser.
// Parse errors throw std::invalid_argument and never return partial content.
WorldMap load_world_map(std::istream& input);
CharacterDefinition load_character_definition(std::istream& input);
ItemDefinition load_item_definition(std::istream& input);
EventDefinition load_event_definition(std::istream& input);
QuestDefinition load_quest_definition(std::istream& input);

} // namespace modernps
