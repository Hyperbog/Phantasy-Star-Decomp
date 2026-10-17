#include "modernps/save.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace modernps {
namespace {
constexpr std::size_t max_bytes = 128 * 1024;

std::uint32_t crc32(std::string_view bytes) noexcept {
    std::uint32_t crc = 0xffffffffu;
    for (const unsigned char c : bytes) {
        crc ^= c;
        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    }
    return crc ^ 0xffffffffu;
}

std::string checksum(std::string_view bytes) {
    std::ostringstream result;
    result << std::hex << std::setfill('0') << std::setw(8) << crc32(bytes);
    return result.str();
}

int decimal(std::string_view value, int lo, int hi) {
    if (value.empty() || value[0] == '+' || value[0] == ' ')
        throw std::invalid_argument("invalid save number");
    int number = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
    if (error != std::errc{} || end != value.data() + value.size() ||
        number < lo || number > hi)
        throw std::invalid_argument("save number outside expected range");
    return number;
}

std::string after(std::string_view line, std::string_view prefix) {
    if (!line.starts_with(prefix)) throw std::invalid_argument("unexpected save field");
    return std::string(line.substr(prefix.size()));
}

std::vector<std::string> split(std::string_view value, std::size_t count) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const auto comma = value.find(',', start);
        fields.emplace_back(value.substr(start, comma == std::string_view::npos
                                              ? comma : comma-start));
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    if (fields.size() != count) throw std::invalid_argument("malformed save record");
    return fields;
}

std::string next(std::istringstream& input) {
    std::string line;
    if (!std::getline(input, line) || line.size() > 256)
        throw std::invalid_argument("truncated or overlong save line");
    for (unsigned char c : line)
        if (c < 0x20 || c == 0x7f)
            throw std::invalid_argument("control character in save");
    return line;
}

std::string token(std::string_view token) {
    if (!valid_content_id(token)) throw std::invalid_argument("bad save identifier");
    return std::string(token);
}

char facing_char(Direction facing) {
    switch (facing) {
        case Direction::north: return '0';
        case Direction::east: return '1';
        case Direction::south: return '2';
        case Direction::west: return '3';
    }
    throw std::invalid_argument("invalid facing in state");
}

std::string slot_id(const std::optional<std::string>& id) {
    return id ? *id : "-";
}

std::optional<std::string> slot_value(std::string_view id) {
    if (id == "-") return std::nullopt;
    return token(id);
}

} // namespace

std::string encode_save(const GameState& state) {
    std::ostringstream out;
    const auto pos = state.world.position();
    out << "PSAVE 1\nmap=" << state.world.map().id() << "\nx=" << pos.x << "\ny=" << pos.y
        << "\nfacing=" << facing_char(state.world.facing())
        << "\nwallet=" << state.wallet.balance()
        << "\nparty=" << state.party.members().size() << '\n';
    for (const auto& character : state.party.members())
        out << "actor=" << character.definition().id << ',' << character.experience()
            << ',' << character.hp() << '\n';
    out << "inventory_capacity=" << state.inventory.capacity()
        << "\ninventory=" << state.inventory.entries().size() << '\n';
    for (const auto& [id, quantity] : state.inventory.entries())
        out << "item=" << id << ',' << quantity << '\n';
    out << "loadouts=" << state.party.members().size() << '\n';
    for (const auto& character : state.party.members()) {
        const auto& id = character.definition().id;
        const auto found = state.equipment.find(id);
        Loadout empty;
        const auto& gear = found == state.equipment.end() ? empty : found->second;
        out << "loadout=" << id << ',' << slot_id(gear.equipped(ItemKind::weapon))
            << ',' << slot_id(gear.equipped(ItemKind::armor))
            << ',' << slot_id(gear.equipped(ItemKind::accessory)) << '\n';
    }
    out << "flags=" << state.events.flags().size() << '\n';
    for (const auto& flag : state.events.flags()) out << "flag=" << flag << '\n';
    out << "completed=" << state.events.completed_events().size() << '\n';
    for (const auto& event : state.events.completed_events()) out << "event=" << event << '\n';
    const auto payload = out.str();
    if (payload.size() + 18 > max_bytes)
        throw std::invalid_argument("save size exceeds limit");
    return payload + "checksum=" + checksum(payload) + '\n';
}

GameState decode_save(std::string_view bytes, const GameState& blueprint,
                      const ItemCatalog& catalog) {
    if (bytes.size() > max_bytes || bytes.size() < 24 || !bytes.ends_with('\n'))
        throw std::invalid_argument("invalid save size or termination");
    const auto marker = bytes.rfind("checksum=");
    if (marker == std::string_view::npos || marker == 0 ||
        bytes[marker-1] != '\n' || bytes.size() != marker + 18 ||
        bytes.substr(marker + 9, 8) != checksum(bytes.substr(0, marker)))
        throw std::invalid_argument("save checksum mismatch");
    const auto payload = bytes.substr(0, marker);
    std::istringstream input{std::string(payload)};
    if (next(input) != "PSAVE 1") throw std::invalid_argument("unsupported save version");
    GameState staged = blueprint;
    const auto map_id = token(after(next(input), "map="));
    if (map_id != staged.world.map().id())
        throw std::invalid_argument("save belongs to another map");
    const int x = decimal(after(next(input), "x="), 0, 127);
    const int y = decimal(after(next(input), "y="), 0, 127);
    const int facing = decimal(after(next(input), "facing="), 0, 3);
    staged.world.restore({x,y}, static_cast<Direction>(facing));
    staged.wallet = Wallet(decimal(after(next(input), "wallet="), 0, Wallet::max_money));
    const int party_size = decimal(after(next(input), "party="), 1, 4);
    if (static_cast<std::size_t>(party_size) != staged.party.members().size())
        throw std::invalid_argument("party roster differs from content blueprint");
    for (const auto& member : blueprint.party.members()) {
        const auto fields = split(after(next(input), "actor="), 3);
        if (token(fields[0]) != member.definition().id)
            throw std::invalid_argument("character roster mismatch");
        staged.party.member(fields[0]).restore_progress(
            decimal(fields[1], 0, Character::max_experience),
            decimal(fields[2], 0, 1'000'000));
    }
    const int capacity = decimal(after(next(input), "inventory_capacity="), 1, 1000);
    if (static_cast<std::size_t>(capacity) != blueprint.inventory.capacity())
        throw std::invalid_argument("inventory capacity differs from blueprint");
    staged.inventory = Inventory(static_cast<std::size_t>(capacity));
    const int item_count = decimal(after(next(input), "inventory="), 0, capacity);
    std::set<std::string> seen;
    for (int i = 0; i < item_count; ++i) {
        const auto fields = split(after(next(input), "item="), 2);
        const auto id = token(fields[0]);
        if (!seen.insert(id).second) throw std::invalid_argument("duplicate inventory item");
        staged.inventory.add(catalog, id, decimal(fields[1], 1, 99));
    }
    if (decimal(after(next(input), "loadouts="), 1, 4) != party_size)
        throw std::invalid_argument("equipment roster mismatch");
    staged.equipment.clear();
    for (const auto& member : blueprint.party.members()) {
        const auto fields = split(after(next(input), "loadout="), 4);
        const auto id = token(fields[0]);
        if (id != member.definition().id) throw std::invalid_argument("equipment character mismatch");
        const std::array<std::optional<std::string>, 3> slots = {
            slot_value(fields[1]), slot_value(fields[2]), slot_value(fields[3])};
        auto loadout = Loadout::restore(catalog, slots);
        (void)loadout.effective_stats(staged.party.member(id), catalog);
        staged.equipment.emplace(id, std::move(loadout));
    }
    std::set<std::string, std::less<>> flags;
    const int flag_count = decimal(after(next(input), "flags="), 0, 4096);
    for (int i = 0; i < flag_count; ++i)
        if (!flags.insert(token(after(next(input), "flag="))).second)
            throw std::invalid_argument("duplicate save flag");
    std::set<std::string, std::less<>> completed;
    const int event_count = decimal(after(next(input), "completed="), 0, 4096);
    for (int i = 0; i < event_count; ++i)
        if (!completed.insert(token(after(next(input), "event="))).second)
            throw std::invalid_argument("duplicate completed event");
    staged.events.restore_progress(flags, completed);
    std::string extra;
    if (std::getline(input, extra)) throw std::invalid_argument("trailing save fields");
    return staged;
}

void write_save_file(const std::filesystem::path& path, const GameState& state) {
    const auto bytes = encode_save(state);
    auto temporary = path;
    temporary += ".tmp";
    if (std::filesystem::exists(temporary))
        throw std::runtime_error("temporary save already exists");
    try {
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out || !out.write(bytes.data(), static_cast<std::streamsize>(bytes.size())) ||
                !out.flush()) throw std::runtime_error("save write failed");
        }
        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

GameState read_save_file(const std::filesystem::path& path,
                         const GameState& blueprint, const ItemCatalog& catalog) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size > max_bytes) throw std::invalid_argument("invalid save file size");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read save file");
    // Allocate no more than the validated maximum, even if a file grows
    // between file_size() and the read. Check exact length and trailing bytes.
    std::string bytes(static_cast<std::size_t>(size), '\0');
    if (!input.read(bytes.data(), static_cast<std::streamsize>(bytes.size())) ||
        input.peek() != std::char_traits<char>::eof() || input.bad())
        throw std::invalid_argument("save file changed or could not be fully read");
    return decode_save(bytes, blueprint, catalog);
}

} // namespace modernps
