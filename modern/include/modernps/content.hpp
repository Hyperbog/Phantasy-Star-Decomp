#pragma once

#include "modernps/character.hpp"
#include "modernps/world.hpp"

#include <iosfwd>

namespace modernps {

// Strict, human-editable, versioned v1 text formats; no third-party parser.
// Parse errors throw std::invalid_argument and never return partial content.
WorldMap load_world_map(std::istream& input);
CharacterDefinition load_character_definition(std::istream& input);

} // namespace modernps
