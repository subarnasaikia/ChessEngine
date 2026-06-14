#pragma once

#include "Types.hpp"
#include "Bitboard.hpp"
#include "Move.hpp"
#include "Attacks.hpp"

#include <string>
#include <vector>

namespace chess {

// Standard starting position in Forsyth-Edwards Notation.
inline const char* STARTPOS_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Information needed to undo a move. One entry is pushed per make_move.
struct StateInfo {
    int       castling;       // castling rights before the move
    Square    epSquare;       // en-passant target before the move
    int       halfmoveClock;  // halfmove clock before the move
    PieceType captured;       // captured piece type (NO_PIECE_TYPE if none)
    uint64_t  key;            // Zobrist key before the move
};

// A full chess position: piece placement plus side to move, castling rights,
// en-passant square, and move clocks. Supports FEN I/O and make/unmake.
class Position {
public:
    Position() { clear(); }

    // Load a position from FEN. Returns false on malformed input.
    bool set(const std::string& fen);
    std::string fen() const;

    // --- Piece queries ------------------------------------------------------
    Piece    piece_on(Square s) const { return board_[s]; }
    bool     empty(Square s)    const { return board_[s] == NO_PIECE; }

    Bitboard pieces()                 const { return byColor_[WHITE] | byColor_[BLACK]; }
    Bitboard pieces(Color c)          const { return byColor_[c]; }
    Bitboard pieces(PieceType pt)     const { return byType_[pt]; }
    Bitboard pieces(Color c, PieceType pt) const { return byColor_[c] & byType_[pt]; }
    Bitboard pieces(PieceType a, PieceType b) const { return byType_[a] | byType_[b]; }

    Square   king_square(Color c) const { return lsb(pieces(c, KING)); }

    // --- State --------------------------------------------------------------
    Color  side_to_move()    const { return sideToMove_; }
    int    castling_rights() const { return castling_; }
    Square ep_square()       const { return epSquare_; }
    int    halfmove_clock()  const { return halfmoveClock_; }
    int    fullmove_number() const { return fullmoveNumber_; }

    bool can_castle(CastlingRight cr) const { return castling_ & cr; }

    // Incrementally-maintained Zobrist hash of the current position.
    uint64_t key() const { return key_; }

    // --- Draw conditions ----------------------------------------------------
    // How many times the current position has occurred so far (including now):
    // 1 = first time, 3 = threefold. Only the last `halfmove_clock()` plies can
    // match, so the scan is bounded by that window.
    int  repetition_count() const;

    // True for dead positions where no checkmate is possible: K vs K,
    // K + single minor vs K, and bishops-only with every bishop on one color.
    bool is_insufficient_material() const;

    // True once the halfmove clock reaches 100 (the fifty-move rule).
    bool is_fifty_move_rule() const { return halfmoveClock_ >= 100; }

    // --- Attacks / checks ---------------------------------------------------
    // All pieces (either color) that attack `s`, given occupancy `occ`.
    Bitboard attackers_to(Square s, Bitboard occ) const;
    bool     is_attacked(Square s, Color by) const;
    bool     in_check() const { return is_attacked(king_square(sideToMove_), ~sideToMove_); }

    // --- Make / unmake ------------------------------------------------------
    void make_move(Move m);
    void unmake_move(Move m);

    std::string to_string() const; // ASCII board, for debugging

private:
    void clear();
    void put_piece(Piece pc, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);

    Bitboard byType_[PIECE_TYPE_NB];
    Bitboard byColor_[COLOR_NB];
    Piece    board_[SQUARE_NB];

    Color  sideToMove_;
    int    castling_;
    Square epSquare_;
    int    halfmoveClock_;
    int    fullmoveNumber_;

    uint64_t key_;                      // Zobrist key of the current position
    std::vector<uint64_t> repKeys_;     // key after each ply, for repetition

    std::vector<StateInfo> history_;
};

} // namespace chess
