#include "modernps/content.hpp"

#include <charconv>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace modernps {
namespace {

std::string line(std::istream& input) {
    std::string result;
    if (!std::getline(input, result)) {
        throw std::invalid_argument("unexpected end of content document");
    }
    if (!result.empty() && result.back() == '\r') result.pop_back();
    if (result.size() > 256) throw std::invalid_argument("content line too long");
    for (const unsigned char c : result) {
        if (c < 0x20 || c == 0x7f) {
            throw std::invalid_argument("control character in content document");
        }
    }
    return result;
}

void header(std::istream& input, std::string_view expected) {
    if (line(input) != expected) {
        throw std::invalid_argument("unsupported content schema or version");
    }
}

std::string field(std::istream& input, std::string_view key) {
    const auto content = line(input);
    const std::string prefix = std::string(key) + '=';
    if (!content.starts_with(prefix)) {
        throw std::invalid_argument("unexpected field (schema order is fixed): " +
                                    std::string(key));
    }
    return content.substr(prefix.size());
}

int number(std::istream& input, std::string_view key, int minimum, int maximum) {
    const auto value = field(input, key);
    int parsed = 0;
    const auto [end, error] = std::from_chars(value.data(),
                                               value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() ||
        parsed < minimum || parsed > maximum) {
        throw std::invalid_argument("invalid numeric field: " + std::string(key));
    }
    return parsed;
}

std::string identifier(std::istream& input) {
    const auto id = field(input, "id");
    if (id.empty() || id.size() > 48 || id.front() < 'a' || id.front() > 'z') {
        throw std::invalid_argument("invalid content id");
    }
    for (const char c : id) {
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
            throw std::invalid_argument("invalid content id");
        }
    }
    return id;
}

void no_trailer(std::istream& input) {
    std::string extra;
    if (std::getline(input, extra)) {
        throw std::invalid_argument("unexpected fields or trailing data");
    }
    if (input.bad()) {
        throw std::invalid_argument("I/O error reading content document");
    }
}

} // namespace

WorldMap load_world_map(std::istream& input) {
    header(input, "PSMAP 1");
    auto id = identifier(input);
    const int width = number(input, "width", 1, 128);
    const int height = number(input, "height", 1, 128);
    const int x = number(input, "spawn_x", 0, width - 1);
    const int y = number(input, "spawn_y", 0, height - 1);
    std::vector<std::string> rows;
    rows.reserve(static_cast<std::size_t>(height));
    for (int row = 0; row != height; ++row) {
        rows.push_back(field(input, "row"));
    }
    no_trailer(input);
    return WorldMap(std::move(id), width, height, std::move(rows), {x, y});
}

CharacterDefinition load_character_definition(std::istream& input) {
    header(input, "PSCHAR 1");
    CharacterDefinition definition;
    definition.id = identifier(input);
    definition.display_name = field(input, "name");
    if (definition.display_name.empty() || definition.display_name.size() > 80) {
        throw std::invalid_argument("invalid character display name");
    }
    definition.base.max_hp = number(input, "max_hp", 1, 1'000'000);
    definition.base.attack = number(input, "attack", 0, 1'000'000);
    definition.base.defense = number(input, "defense", 0, 1'000'000);
    definition.base.speed = number(input, "speed", 0, 1'000'000);
    definition.growth.max_hp = number(input, "growth_hp", 0, 1'000'000);
    definition.growth.attack = number(input, "growth_attack", 0, 1'000'000);
    definition.growth.defense = number(input, "growth_defense", 0, 1'000'000);
    definition.growth.speed = number(input, "growth_speed", 0, 1'000'000);
    no_trailer(input);
    // The engine's full-level validation is part of the content boundary.
    Character checked(definition);
    return definition;
}

} // namespace modernps
