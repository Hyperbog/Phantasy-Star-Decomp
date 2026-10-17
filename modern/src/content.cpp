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


ItemDefinition load_item_definition(std::istream& input) {
    header(input, "PSITEM 1");
    ItemDefinition item;
    item.id = identifier(input);
    item.name = field(input, "name");
    const auto kind = field(input, "kind");
    if (kind == "consumable") item.kind = ItemKind::consumable;
    else if (kind == "weapon") item.kind = ItemKind::weapon;
    else if (kind == "armor") item.kind = ItemKind::armor;
    else if (kind == "accessory") item.kind = ItemKind::accessory;
    else if (kind == "key_item") item.kind = ItemKind::key_item;
    else throw std::invalid_argument("unknown item kind");
    item.max_stack = number(input, "max_stack", 1, 99);
    item.healing = number(input, "healing", 0, 1'000'000);
    item.attack_bonus = number(input, "attack_bonus", 0, 1'000'000);
    item.defense_bonus = number(input, "defense_bonus", 0, 1'000'000);
    item.speed_bonus = number(input, "speed_bonus", 0, 1'000'000);
    no_trailer(input);
    ItemCatalog catalog;
    catalog.add(item); // all cross-field validation is shared with runtime
    return item;
}

EventDefinition load_event_definition(std::istream& input) {
    header(input, "PSEVENT 1");
    EventDefinition definition;
    definition.id = identifier(input);
    definition.requires_flag = field(input, "requires_flag");
    definition.repeatable = number(input, "repeatable", 0, 1) != 0;
    const auto count = number(input, "action_count", 1, 16);
    for (int n = 0; n < count; ++n) {
        const auto value = field(input, "action");
        const auto first = value.find(':');
        const auto second = first == std::string::npos ? first : value.find(':', first + 1);
        if (first == std::string::npos || second == std::string::npos ||
            value.find(':', second + 1) != std::string::npos) {
            throw std::invalid_argument("invalid event action encoding");
        }
        const auto type = value.substr(0, first);
        const auto target = value.substr(first + 1, second - first - 1);
        const auto amount_text = value.substr(second + 1);
        int amount = 0;
        const auto [end, error] = std::from_chars(amount_text.data(),
                                    amount_text.data() + amount_text.size(), amount);
        if (error != std::errc{} || end != amount_text.data() + amount_text.size())
            throw std::invalid_argument("invalid event action amount");
        EventActionKind kind;
        if (type == "set_flag") kind = EventActionKind::set_flag;
        else if (type == "give_item") kind = EventActionKind::give_item;
        else if (type == "grant_experience") kind = EventActionKind::grant_experience;
        else throw std::invalid_argument("unknown event action type");
        definition.actions.push_back({kind, target, amount});
    }
    no_trailer(input);
    EventEngine engine;
    engine.add_event(definition);
    return definition;
}

QuestDefinition load_quest_definition(std::istream& input) {
    header(input, "PSQUEST 1");
    QuestDefinition definition;
    definition.id = identifier(input);
    definition.title = field(input, "title");
    definition.activation_flag = field(input, "activation_flag");
    definition.completion_flag = field(input, "completion_flag");
    no_trailer(input);
    QuestJournal journal;
    journal.add_quest(definition);
    return definition;
}


NpcDefinition load_npc_definition(std::istream& input) {
    header(input, "PSNPC 1");
    NpcDefinition npc;
    npc.id = identifier(input);
    npc.name = field(input, "name");
    const int count = number(input, "lines", 1, 16);
    for (int i = 0; i < count; ++i) {
        auto definition = field(input, "line");
        const auto delim = definition.find('|');
        if (delim == std::string::npos || definition.find('|', delim+1) != std::string::npos)
            throw std::invalid_argument("malformed NPC dialogue line");
        auto flag = definition.substr(0, delim);
        if (flag == "-") flag.clear();
        npc.dialogue.push_back({flag, definition.substr(delim+1)});
    }
    no_trailer(input);
    DialogueEngine checked;
    checked.add_npc(npc);
    return npc;
}

ShopDefinition load_shop_definition(std::istream& input, const ItemCatalog& catalog) {
    header(input, "PSSHOP 1");
    ShopDefinition shop;
    shop.id = identifier(input);
    const int count = number(input, "offers", 1, 32);
    for (int i = 0; i < count; ++i) {
        const auto entry = field(input, "offer");
        const auto a = entry.find(',');
        const auto b = entry.find(',', a == std::string::npos ? a : a+1);
        if (a == std::string::npos || b == std::string::npos ||
            entry.find(',', b+1) != std::string::npos)
            throw std::invalid_argument("malformed shop offer");
        auto parse = [](std::string_view value) {
            int n = 0;
            const auto [end, error] = std::from_chars(value.data(),
                value.data()+value.size(), n);
            if (error != std::errc{} || end != value.data()+value.size())
                throw std::invalid_argument("invalid shop price");
            return n;
        };
        shop.offers.push_back({entry.substr(0,a),
                               parse(std::string_view(entry).substr(a+1,b-a-1)),
                               parse(std::string_view(entry).substr(b+1))});
    }
    no_trailer(input);
    Shop checked(shop, catalog);
    return shop;
}

} // namespace modernps
