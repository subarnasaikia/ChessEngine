#pragma once

#include "Position.hpp"
#include <cstdint>

namespace chess {
namespace perft {

// One-time engine initialization (builds attack tables). Safe to call repeatedly.
void init();

// Count the number of leaf nodes in the move tree of `pos` to the given depth.
// Only fully legal moves are counted (pseudo-legal moves leaving the own king
// in check are filtered via make/unmake).
uint64_t perft(Position& pos, int depth);

// Like perft, but prints each root move with its subtree node count ("divide"),
// then the total. Useful for locating move-generation discrepancies.
uint64_t divide(Position& pos, int depth);

} // namespace perft
} // namespace chess
