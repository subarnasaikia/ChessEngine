#pragma once

#include <cstdint>

namespace chess {

// ---------------------------------------------------------------------------
// Colors
// ---------------------------------------------------------------------------
enum Color : int { WHITE, BLACK, COLOR_NB = 2 };

constexpr Color operator~(Color c) { return Color(c ^ BLACK); }

// ---------------------------------------------------------------------------
// Piece types and pieces
//
// A Piece packs color and type into a single byte: (color << 3) | type.
// This leaves a small gap (indices 6, 7) but keeps type_of()/color_of() to a
// single shift/mask, which is convenient in make/unmake.
// ---------------------------------------------------------------------------
enum PieceType : int {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 6,
    NO_PIECE_TYPE = 7
};

enum Piece : int {
    W_PAWN = 0, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    NO_PIECE = 16,
    PIECE_NB = 16
};

constexpr Piece make_piece(Color c, PieceType pt) { return Piece((c << 3) | pt); }
constexpr PieceType type_of(Piece pc) { return PieceType(pc & 7); }
constexpr Color color_of(Piece pc) { return Color(pc >> 3); }

// ---------------------------------------------------------------------------
// Squares, files, ranks
//
// Little-Endian Rank-File mapping (LERF): square = rank * 8 + file.
//   A1 = 0, B1 = 1, ... H1 = 7, A2 = 8, ... H8 = 63.
// ---------------------------------------------------------------------------
enum Square : int {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQ_NONE = 64,
    SQUARE_NB = 64
};

enum File : int { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB = 8 };
enum Rank : int { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB = 8 };

constexpr Square make_square(File f, Rank r) { return Square((r << 3) | f); }
constexpr File file_of(Square s) { return File(s & 7); }
constexpr Rank rank_of(Square s) { return Rank(s >> 3); }

// Flip a square vertically (rank 1 <-> rank 8). Useful for mirrored tables.
constexpr Square flip_rank(Square s) { return Square(s ^ 56); }

constexpr bool is_ok(Square s) { return s >= A1 && s <= H8; }

// ---------------------------------------------------------------------------
// Castling rights (bit flags)
// ---------------------------------------------------------------------------
enum CastlingRight : int {
    NO_CASTLING = 0,
    WHITE_OO  = 1,
    WHITE_OOO = 2,
    BLACK_OO  = 4,
    BLACK_OOO = 8,
    ANY_CASTLING = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO
};

// ---------------------------------------------------------------------------
// Directions, expressed as square-index deltas (used carefully, with file
// masks, to avoid board wrap-around).
// ---------------------------------------------------------------------------
enum Direction : int {
    NORTH =  8,
    SOUTH = -8,
    EAST  =  1,
    WEST  = -1,
    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST
};

} // namespace chess
