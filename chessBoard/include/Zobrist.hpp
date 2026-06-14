#pragma once

#include "Types.hpp"
#include <cstdint>

namespace chess {
namespace zobrist {

// Random constants used to hash a position into a 64-bit Zobrist key. The key
// is maintained incrementally by Position (XOR in/out as pieces, side-to-move,
// castling rights, and the en-passant file change) and underpins repetition
// detection today and transposition tables in any future search.
//
// `psq` is indexed by the Piece enum directly (which has gaps at 6,7,14 and
// uses 16 for NO_PIECE); the unused slots are simply never read.
extern uint64_t psq[PIECE_NB][SQUARE_NB];
extern uint64_t side;            // XORed in when it is Black to move
extern uint64_t castling[16];    // one value per 4-bit castling-rights mask
extern uint64_t epFile[FILE_NB]; // one value per en-passant file

// Fill the tables from a fixed-seed PRNG (deterministic across runs, like the
// magic-number search in Attacks.cpp). Idempotent: safe to call repeatedly.
// Position::set runs it automatically, so callers rarely invoke it directly.
void init();

} // namespace zobrist
} // namespace chess
