#pragma once

#include "Types.hpp"
#include <cstdint>
#include <string>

namespace chess {

// ---------------------------------------------------------------------------
// A move is packed into 16 bits:
//   bits  0-5 : destination square (0-63)
//   bits  6-11: origin square (0-63)
//   bits 12-13: promotion piece type, encoded as (PieceType - KNIGHT):
//               0 = KNIGHT, 1 = BISHOP, 2 = ROOK, 3 = QUEEN
//   bits 14-15: move type flag (see MoveType)
//
// The promotion-type bits are only meaningful when the flag is PROMOTION.
// ---------------------------------------------------------------------------

enum MoveType : uint16_t {
    NORMAL      = 0,
    PROMOTION   = 1 << 14,
    EN_PASSANT  = 2 << 14,
    CASTLING    = 3 << 14
};

class Move {
public:
    Move() = default;
    constexpr explicit Move(uint16_t data) : data_(data) {}

    constexpr Move(Square from, Square to)
        : data_(uint16_t((from << 6) | to)) {}

    // Construct a typed move. For PROMOTION pass the target piece type.
    template <MoveType T>
    static constexpr Move make(Square from, Square to, PieceType promo = KNIGHT) {
        return Move(uint16_t(T | ((promo - KNIGHT) << 12) | (from << 6) | to));
    }

    constexpr Square from() const { return Square((data_ >> 6) & 0x3F); }
    constexpr Square to()   const { return Square(data_ & 0x3F); }
    constexpr MoveType type() const { return MoveType(data_ & (3 << 14)); }
    constexpr PieceType promotion_type() const {
        return PieceType(((data_ >> 12) & 3) + KNIGHT);
    }

    constexpr uint16_t raw() const { return data_; }

    // A null/none move (from == to == A1 can never be legal).
    static constexpr Move none() { return Move(0); }
    constexpr bool is_ok() const { return data_ != 0; }

    constexpr bool operator==(const Move& m) const { return data_ == m.data_; }
    constexpr bool operator!=(const Move& m) const { return data_ != m.data_; }

private:
    uint16_t data_ = 0;
};

// Long algebraic notation, e.g. "e2e4", "e7e8q", "e1g1".
std::string to_uci(Move m);

// ---------------------------------------------------------------------------
// A fixed-capacity move list. 256 is comfortably above the maximum number of
// legal moves in any reachable position (~218).
// ---------------------------------------------------------------------------
struct MoveList {
    Move moves[256];
    int  count = 0;

    void add(Move m) { moves[count++] = m; }

    Move*       begin()       { return moves; }
    Move*       end()         { return moves + count; }
    const Move* begin() const { return moves; }
    const Move* end()   const { return moves + count; }
    int         size()  const { return count; }
};

} // namespace chess
