#include "modernps/world.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace modernps;

namespace {
void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}
template <typename Error, typename Fn>
void rejects(Fn&& operation) {
    bool caught = false;
    try { operation(); } catch (const Error&) { caught = true; }
    require(caught, "invalid input was accepted");
}
WorldMap sample() {
    return {"test", 5, 4, {"#####", "#...#", "#..##", "#####"}, {1, 1}};
}
void movement() {
    WorldState state(sample());
    require(state.position() == Position{1, 1}, "incorrect spawn");
    const auto first = state.move(Direction::east);
    require(first.moved() && first.from == Position{1, 1} &&
            first.to == Position{2, 1}, "valid east step failed");
    require(state.facing() == Direction::east, "facing not updated");
    require(state.move(Direction::south).moved(), "valid south step failed");
    require(state.position() == Position{2, 2}, "south destination wrong");
}
void wall_blocks_but_turns() {
    WorldState state(sample());
    const auto move = state.move(Direction::north);
    require(!move.moved() && move.blocked_by == MoveBlock::wall,
            "wall was not detected");
    require(state.facing() == Direction::north && state.position() == Position{1, 1},
            "wall contact incorrectly moved or failed to rotate");
}
void bound_blocks() {
    WorldState state(WorldMap("edge", 2, 1, {".."}, {0, 0}));
    const auto move = state.move(Direction::west);
    require(!move.moved() && move.blocked_by == MoveBlock::bounds &&
            state.position() == Position{0, 0}, "bounds check failed");
}
void invalid_spawn() {
    rejects<std::invalid_argument>([] {
        WorldMap map("blocked", 1, 1, {"#"}, {0, 0});
    });
    rejects<std::invalid_argument>([] {
        WorldMap map("outside", 1, 1, {"."}, {-1, 0});
    });
}
void invalid_map() {
    rejects<std::invalid_argument>([] {
        WorldMap map("bad", 2, 1, {".X"}, {0, 0});
    });
    rejects<std::invalid_argument>([] {
        WorldMap map("bad", 2, 1, {"."}, {0, 0});
    });
    rejects<std::invalid_argument>([] {
        WorldMap map("bad", 2, 2, {".."}, {0, 0});
    });
    rejects<std::invalid_argument>([] {
        WorldMap map("huge", 129, 1, {"."}, {0, 0});
    });
    rejects<std::invalid_argument>([] {
        WorldMap map("", 1, 1, {"."}, {0, 0});
    });
}
void invalid_direction_is_atomic() {
    WorldState state(sample());
    rejects<std::invalid_argument>([&] {
        state.move(static_cast<Direction>(99));
    });
    require(state.position() == Position{1, 1} &&
            state.facing() == Direction::south,
            "invalid direction mutated state");
    rejects<std::invalid_argument>([] {
        WorldState state(sample(), static_cast<Direction>(99));
    });
}
void tile_lookup() {
    const auto map = sample();
    require(map.tile({0, 0}) == '#' && map.tile({1, 1}) == '.',
            "incorrect tile lookup");
    require(!map.contains({-1, 0}) && !map.walkable({-1, 0}),
            "off-map access accepted");
    rejects<std::out_of_range>([&] { static_cast<void>(map.tile({9, 9})); });
}
} // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> checks = {
        {"movement", movement}, {"wall_blocks_but_turns", wall_blocks_but_turns},
        {"bound_blocks", bound_blocks}, {"invalid_spawn", invalid_spawn},
        {"invalid_map", invalid_map}, {"tile_lookup", tile_lookup},
        {"invalid_direction_is_atomic", invalid_direction_is_atomic}
    };
    try {
        for (const auto& [name, check] : checks) {
            check();
            std::cout << "PASS " << name << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
