#pragma once

#include "Position.hpp"
#include "Move.hpp"

namespace chess {
namespace movegen {

// Append all pseudo-legal moves for the side to move to `list`.
//
// "Pseudo-legal" means moves are geometrically and rule-valid but may leave
// the mover's own king in check; callers filter those out via make_move +
// in-check test. Castling moves, however, are fully validated here (the king
// may not be in check, pass through, or land on an attacked square), since
// the pass-through square is not covered by a post-move king-safety test.
void generate(const Position& pos, MoveList& list);

} // namespace movegen
} // namespace chess
