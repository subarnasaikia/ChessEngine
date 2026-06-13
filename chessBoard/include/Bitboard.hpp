#pragma once

#include "Types.hpp"
#include <cstdint>
#include <string>

namespace chess {

// A bitboard is a 64-bit word, one bit per square (LERF: bit 0 = A1).
using Bitboard = uint64_t;

constexpr Bitboard EMPTY_BB = 0ULL;
constexpr Bitboard FULL_BB  = ~0ULL;

// File and rank masks.
constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
constexpr Bitboard FILE_B_BB = FILE_A_BB << 1;
constexpr Bitboard FILE_C_BB = FILE_A_BB << 2;
constexpr Bitboard FILE_D_BB = FILE_A_BB << 3;
constexpr Bitboard FILE_E_BB = FILE_A_BB << 4;
constexpr Bitboard FILE_F_BB = FILE_A_BB << 5;
constexpr Bitboard FILE_G_BB = FILE_A_BB << 6;
constexpr Bitboard FILE_H_BB = FILE_A_BB << 7;

constexpr Bitboard RANK_1_BB = 0xFFULL;
constexpr Bitboard RANK_2_BB = RANK_1_BB << (8 * 1);
constexpr Bitboard RANK_3_BB = RANK_1_BB << (8 * 2);
constexpr Bitboard RANK_4_BB = RANK_1_BB << (8 * 3);
constexpr Bitboard RANK_5_BB = RANK_1_BB << (8 * 4);
constexpr Bitboard RANK_6_BB = RANK_1_BB << (8 * 5);
constexpr Bitboard RANK_7_BB = RANK_1_BB << (8 * 6);
constexpr Bitboard RANK_8_BB = RANK_1_BB << (8 * 7);

constexpr Bitboard square_bb(Square s) { return 1ULL << s; }

inline Bitboard file_bb(File f) { return FILE_A_BB << f; }
inline Bitboard rank_bb(Rank r) { return RANK_1_BB << (8 * r); }

// Set/test/clear individual squares.
inline bool     more_than_one(Bitboard b) { return b & (b - 1); }
constexpr bool  test_bit(Bitboard b, Square s) { return (b >> s) & 1; }

// ---------------------------------------------------------------------------
// Population count and bit scans (GCC/Clang builtins).
// ---------------------------------------------------------------------------
inline int popcount(Bitboard b) { return __builtin_popcountll(b); }

// Index of the least significant 1-bit. Undefined for b == 0.
inline Square lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }

// Index of the most significant 1-bit. Undefined for b == 0.
inline Square msb(Bitboard b) { return Square(63 ^ __builtin_clzll(b)); }

// Return the LSB index and clear it from the set.
inline Square pop_lsb(Bitboard& b) {
    const Square s = lsb(b);
    b &= b - 1;
    return s;
}

// ---------------------------------------------------------------------------
// Wrap-safe directional shifts of an entire bitboard.
// Shifting east/west masks off the file that would wrap around.
// ---------------------------------------------------------------------------
template <Direction D>
constexpr Bitboard shift(Bitboard b) {
    return D == NORTH      ? b << 8
         : D == SOUTH      ? b >> 8
         : D == EAST       ? (b & ~FILE_H_BB) << 1
         : D == WEST       ? (b & ~FILE_A_BB) >> 1
         : D == NORTH_EAST ? (b & ~FILE_H_BB) << 9
         : D == NORTH_WEST ? (b & ~FILE_A_BB) << 7
         : D == SOUTH_EAST ? (b & ~FILE_H_BB) >> 7
         : D == SOUTH_WEST ? (b & ~FILE_A_BB) >> 9
         : 0;
}

// Pawn attack spans for an entire set of pawns of a given color.
template <Color C>
constexpr Bitboard pawn_attacks_bb(Bitboard b) {
    return C == WHITE
        ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
        : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}

// Pretty-printer for debugging (rank 8 on top).
std::string to_string(Bitboard b);

} // namespace chess
