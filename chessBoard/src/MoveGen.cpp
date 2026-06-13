#include "MoveGen.hpp"

namespace chess {
namespace movegen {

namespace {

void add_promotions(MoveList& list, Square from, Square to) {
    list.add(Move::make<PROMOTION>(from, to, QUEEN));
    list.add(Move::make<PROMOTION>(from, to, ROOK));
    list.add(Move::make<PROMOTION>(from, to, BISHOP));
    list.add(Move::make<PROMOTION>(from, to, KNIGHT));
}

template <Color Us>
void gen_pawns(const Position& pos, MoveList& list) {
    constexpr Color Them = ~Us;
    constexpr Bitboard PromoRank = (Us == WHITE) ? RANK_8_BB : RANK_1_BB;
    constexpr Bitboard DblRank   = (Us == WHITE) ? RANK_3_BB : RANK_6_BB;
    constexpr int Up = (Us == WHITE) ? 8 : -8;

    const Bitboard pawns = pos.pieces(Us, PAWN);
    const Bitboard empty = ~pos.pieces();
    const Bitboard enemy = pos.pieces(Them);

    // --- Pushes -------------------------------------------------------------
    Bitboard single = (Us == WHITE ? pawns << 8 : pawns >> 8) & empty;
    Bitboard dbl    = (Us == WHITE ? (single & DblRank) << 8
                                   : (single & DblRank) >> 8) & empty;

    Bitboard pushQuiet = single & ~PromoRank;
    Bitboard pushPromo = single & PromoRank;

    while (pushQuiet) {
        const Square to = pop_lsb(pushQuiet);
        list.add(Move(Square(to - Up), to));
    }
    while (dbl) {
        const Square to = pop_lsb(dbl);
        list.add(Move(Square(to - 2 * Up), to));
    }
    while (pushPromo) {
        const Square to = pop_lsb(pushPromo);
        add_promotions(list, Square(to - Up), to);
    }

    // --- Captures (two diagonals) ------------------------------------------
    auto emit_captures = [&](Bitboard caps, int delta) {
        Bitboard quiet = caps & ~PromoRank;
        Bitboard promo = caps & PromoRank;
        while (quiet) {
            const Square to = pop_lsb(quiet);
            list.add(Move(Square(to - delta), to));
        }
        while (promo) {
            const Square to = pop_lsb(promo);
            add_promotions(list, Square(to - delta), to);
        }
    };

    if (Us == WHITE) {
        emit_captures(shift<NORTH_WEST>(pawns) & enemy, NORTH_WEST); // +7
        emit_captures(shift<NORTH_EAST>(pawns) & enemy, NORTH_EAST); // +9
    } else {
        emit_captures(shift<SOUTH_WEST>(pawns) & enemy, SOUTH_WEST); // -9
        emit_captures(shift<SOUTH_EAST>(pawns) & enemy, SOUTH_EAST); // -7
    }

    // --- En passant ---------------------------------------------------------
    if (pos.ep_square() != SQ_NONE) {
        const Square ep = pos.ep_square();
        Bitboard movers = attacks::pawn(Them, ep) & pawns;
        while (movers) {
            const Square from = pop_lsb(movers);
            list.add(Move::make<EN_PASSANT>(from, ep));
        }
    }
}

template <PieceType Pt>
void gen_pieces(const Position& pos, MoveList& list, Bitboard targets) {
    const Color us = pos.side_to_move();
    const Bitboard occ = pos.pieces();
    Bitboard from = pos.pieces(us, Pt);
    while (from) {
        const Square s = pop_lsb(from);
        Bitboard att = attacks::of(Pt, s, occ) & targets;
        while (att)
            list.add(Move(s, pop_lsb(att)));
    }
}

template <Color Us>
void gen_castling(const Position& pos, MoveList& list) {
    constexpr Color Them = ~Us;
    constexpr Square Ksq = (Us == WHITE) ? E1 : E8;
    constexpr CastlingRight KRight = (Us == WHITE) ? WHITE_OO : BLACK_OO;
    constexpr CastlingRight QRight = (Us == WHITE) ? WHITE_OOO : BLACK_OOO;

    // King may not be in check before castling.
    if (pos.is_attacked(Ksq, Them))
        return;

    if (pos.can_castle(KRight)) {
        constexpr Square F = (Us == WHITE) ? F1 : F8;
        constexpr Square G = (Us == WHITE) ? G1 : G8;
        if (pos.empty(F) && pos.empty(G)
            && !pos.is_attacked(F, Them) && !pos.is_attacked(G, Them))
            list.add(Move::make<CASTLING>(Ksq, G));
    }
    if (pos.can_castle(QRight)) {
        constexpr Square D = (Us == WHITE) ? D1 : D8;
        constexpr Square C = (Us == WHITE) ? C1 : C8;
        constexpr Square B = (Us == WHITE) ? B1 : B8;
        if (pos.empty(D) && pos.empty(C) && pos.empty(B)
            && !pos.is_attacked(D, Them) && !pos.is_attacked(C, Them))
            list.add(Move::make<CASTLING>(Ksq, C));
    }
}

template <Color Us>
void generate_all(const Position& pos, MoveList& list) {
    // Targets for non-pawn pieces: any square not occupied by a friendly piece.
    const Bitboard targets = ~pos.pieces(Us);

    gen_pawns<Us>(pos, list);
    gen_pieces<KNIGHT>(pos, list, targets);
    gen_pieces<BISHOP>(pos, list, targets);
    gen_pieces<ROOK>(pos, list, targets);
    gen_pieces<QUEEN>(pos, list, targets);
    gen_pieces<KING>(pos, list, targets);
    gen_castling<Us>(pos, list);
}

} // namespace

void generate(const Position& pos, MoveList& list) {
    if (pos.side_to_move() == WHITE)
        generate_all<WHITE>(pos, list);
    else
        generate_all<BLACK>(pos, list);
}

} // namespace movegen
} // namespace chess
