#pragma once

#include "Position.hpp"

namespace chessbot {

// Static evaluation in centipawns from the side-to-move's perspective
// (positive ⇒ the side to move is better). Negamax-friendly: evaluate(pos)
// for a position and its color-mirror are negatives of each other.
//
// Terms: material + tapered piece-square tables (PeSTO), mobility, king
// shelter, and pawn structure (doubled / isolated / passed). Requires
// attacks::init() to have run.
int evaluate(const chess::Position& pos);

} // namespace chessbot
