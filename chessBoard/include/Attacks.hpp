#pragma once

#include "Types.hpp"
#include "Bitboard.hpp"

namespace chess {

// Precomputed leaping-piece attack tables and magic-bitboard sliding attacks.
//
// attacks::init() MUST be called once at program start, before any attack
// lookup or Position is used. It fills the leaper tables and searches for the
// rook/bishop magics (deterministically, using a fixed PRNG seed).
namespace attacks {

void init();

// Leaping pieces.
Bitboard pawn(Color c, Square s);   // capture squares only
Bitboard knight(Square s);
Bitboard king(Square s);

// Sliding pieces, given a full-board occupancy.
Bitboard bishop(Square s, Bitboard occupied);
Bitboard rook(Square s, Bitboard occupied);
Bitboard queen(Square s, Bitboard occupied);

// Generic dispatch by piece type (PAWN is not handled here; use pawn()).
Bitboard of(PieceType pt, Square s, Bitboard occupied);

} // namespace attacks
} // namespace chess
