#include "modernps/world.hpp"

#include <stdexcept>
#include <utility>

namespace modernps {
namespace {
void verify_direction(Direction direction) {
    switch (direction) {
        case Direction::north:
        case Direction::east:
        case Direction::south:
        case Direction::west:
            return;
    }
    throw std::invalid_argument("invalid direction enum");
}
} // namespace

WorldMap::WorldMap(std::string id, int width, int height,
                   std::vector<std::string> rows, Position spawn)
    : id_(std::move(id)), width_(width), height_(height),
      rows_(std::move(rows)), spawn_(spawn) {
    if (id_.empty() || width_ < 1 || height_ < 1 || width_ > 128 ||
        height_ > 128 || rows_.size() != static_cast<std::size_t>(height_)) {
        throw std::invalid_argument("invalid map identifier, dimensions or row count");
    }
    for (const auto& row : rows_) {
        if (row.size() != static_cast<std::size_t>(width_) ||
            row.find_first_not_of(".#") != std::string::npos) {
            throw std::invalid_argument("invalid map tiles or row width");
        }
    }
    if (!walkable(spawn_)) {
        throw std::invalid_argument("map spawn must be on walkable terrain");
    }
}

bool WorldMap::contains(Position p) const noexcept {
    return p.x >= 0 && p.y >= 0 && p.x < width_ && p.y < height_;
}

bool WorldMap::walkable(Position p) const noexcept {
    return contains(p) && rows_[static_cast<std::size_t>(p.y)]
                                  [static_cast<std::size_t>(p.x)] == '.';
}

char WorldMap::tile(Position p) const {
    if (!contains(p)) throw std::out_of_range("tile outside map bounds");
    return rows_[static_cast<std::size_t>(p.y)]
                [static_cast<std::size_t>(p.x)];
}

WorldState::WorldState(WorldMap map, Direction facing)
    : map_(std::move(map)), position_(map_.spawn()), facing_(facing) {
    verify_direction(facing);
}

MoveResult WorldState::move(Direction direction) {
    verify_direction(direction);
    // Turning occurs even when a step is blocked; position does not.
    facing_ = direction;
    Position next = position_;
    switch (direction) {
        case Direction::north: --next.y; break;
        case Direction::east:  ++next.x; break;
        case Direction::south: ++next.y; break;
        case Direction::west:  --next.x; break;
    }
    MoveBlock reason = MoveBlock::none;
    if (!map_.contains(next)) reason = MoveBlock::bounds;
    else if (!map_.walkable(next)) reason = MoveBlock::wall;
    const Position before = position_;
    if (reason == MoveBlock::none) position_ = next;
    return {before, position_, facing_, reason};
}

} // namespace modernps
