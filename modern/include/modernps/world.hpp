#pragma once

#include <string>
#include <vector>

namespace modernps {

struct Position {
    int x;
    int y;
    bool operator==(const Position&) const = default;
};

enum class Direction { north, east, south, west };
enum class MoveBlock { none, wall, bounds };

struct MoveResult {
    Position from;
    Position to;
    Direction facing;
    MoveBlock blocked_by;
    bool moved() const noexcept { return blocked_by == MoveBlock::none; }
};

// A content-defined top-down grid, not an extraction of an original SMS map.
class WorldMap {
public:
    WorldMap(std::string id, int width, int height,
             std::vector<std::string> rows, Position spawn);
    const std::string& id() const noexcept { return id_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    Position spawn() const noexcept { return spawn_; }
    bool contains(Position position) const noexcept;
    bool walkable(Position position) const noexcept;
    char tile(Position position) const;
private:
    std::string id_;
    int width_;
    int height_;
    std::vector<std::string> rows_;
    Position spawn_;
};

class WorldState {
public:
    explicit WorldState(WorldMap map, Direction facing = Direction::south);
    const WorldMap& map() const noexcept { return map_; }
    Position position() const noexcept { return position_; }
    Direction facing() const noexcept { return facing_; }
    MoveResult move(Direction direction);
private:
    WorldMap map_;
    Position position_;
    Direction facing_;
};

} // namespace modernps
