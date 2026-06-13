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

// Append only the fully legal moves for the side to move. Generates pseudo-legal
// moves and keeps those that do not leave the mover's own king in check, using
// make_move/unmake_move. `pos` is mutated transiently but restored on return.
void generate_legal(Position& pos, MoveList& list);

} // namespace movegen
} // namespace chess
