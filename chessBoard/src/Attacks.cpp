#include "Attacks.hpp"

#include <cassert>

namespace chess {
namespace attacks {

namespace {

// Leaping-piece tables.
Bitboard PawnTable[COLOR_NB][SQUARE_NB];
Bitboard KnightTable[SQUARE_NB];
Bitboard KingTable[SQUARE_NB];

// A fancy-magic entry: relevant-occupancy mask, magic multiplier, the shift
// (64 - relevant bits), and a pointer into the shared attack pool.
struct Magic {
    Bitboard  mask;
    Bitboard  magic;
    Bitboard* attacks;
    unsigned  shift;

    unsigned index(Bitboard occupied) const {
        return unsigned(((occupied & mask) * magic) >> shift);
    }
};

Magic RookMagics[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];

// Shared attack pools. Sizes are the well-known totals for 8x8 fancy magics.
Bitboard RookPool[102400];
Bitboard BishopPool[5248];

// Deterministic xorshift PRNG, used to search for magic numbers.
struct PRNG {
    uint64_t s;
    explicit PRNG(uint64_t seed) : s(seed) {}
    uint64_t next() {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return s * 2685821657736338717ULL;
    }
    // Magic candidates work best when sparse (few bits set).
    uint64_t sparse() { return next() & next() & next(); }
};

// True if (file, rank) is a valid board coordinate.
inline bool on_board(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }

inline Bitboard bb(int f, int r) {
    return square_bb(make_square(File(f), Rank(r)));
}

// Sliding attacks computed on the fly: walk each direction until a blocker
// (inclusive) or the board edge. Used only during initialization.
Bitboard sliding_attack(PieceType pt, Square sq, Bitboard occupied) {
    static const int rookDir[4][2]   = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    static const int bishopDir[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    const auto& dirs = (pt == ROOK) ? rookDir : bishopDir;

    Bitboard attacksBB = 0;
    const int f0 = file_of(sq), r0 = rank_of(sq);
    for (int d = 0; d < 4; ++d) {
        int f = f0 + dirs[d][0];
        int r = r0 + dirs[d][1];
        while (on_board(f, r)) {
            const Bitboard s = bb(f, r);
            attacksBB |= s;
            if (occupied & s) break;
            f += dirs[d][0];
            r += dirs[d][1];
        }
    }
    return attacksBB;
}

// Relevant-occupancy mask: the squares whose contents affect the attack set,
// excluding board-edge squares (a blocker on the edge changes nothing beyond).
Bitboard slider_mask(PieceType pt, Square sq) {
    Bitboard mask = 0;
    const int f0 = file_of(sq), r0 = rank_of(sq);
    if (pt == ROOK) {
        for (int r = r0 + 1; r <= 6; ++r) mask |= bb(f0, r);
        for (int r = r0 - 1; r >= 1; --r) mask |= bb(f0, r);
        for (int f = f0 + 1; f <= 6; ++f) mask |= bb(f, r0);
        for (int f = f0 - 1; f >= 1; --f) mask |= bb(f, r0);
    } else {
        for (int f = f0 + 1, r = r0 + 1; f <= 6 && r <= 6; ++f, ++r) mask |= bb(f, r);
        for (int f = f0 + 1, r = r0 - 1; f <= 6 && r >= 1; ++f, --r) mask |= bb(f, r);
        for (int f = f0 - 1, r = r0 + 1; f >= 1 && r <= 6; --f, ++r) mask |= bb(f, r);
        for (int f = f0 - 1, r = r0 - 1; f >= 1 && r >= 1; --f, --r) mask |= bb(f, r);
    }
    return mask;
}

void init_leapers() {
    static const int knightD[8][2] =
        {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    static const int kingD[8][2] =
        {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

    for (int sq = 0; sq < SQUARE_NB; ++sq) {
        const int f0 = file_of(Square(sq)), r0 = rank_of(Square(sq));

        Bitboard n = 0, k = 0;
        for (int i = 0; i < 8; ++i) {
            int fn = f0 + knightD[i][0], rn = r0 + knightD[i][1];
            if (on_board(fn, rn)) n |= bb(fn, rn);
            int fk = f0 + kingD[i][0], rk = r0 + kingD[i][1];
            if (on_board(fk, rk)) k |= bb(fk, rk);
        }
        KnightTable[sq] = n;
        KingTable[sq]   = k;

        // Pawn capture squares.
        const Bitboard self = square_bb(Square(sq));
        PawnTable[WHITE][sq] = pawn_attacks_bb<WHITE>(self);
        PawnTable[BLACK][sq] = pawn_attacks_bb<BLACK>(self);
    }
}

// Find magics for one slider type, populating its table and attack pool.
void init_magics(PieceType pt, Bitboard* pool, Magic magics[SQUARE_NB]) {
    PRNG rng(0x246CCB7E5FA44F11ULL); // fixed seed -> reproducible magics

    Bitboard occ[4096];
    Bitboard ref[4096];
    int      epoch[4096] = {0};
    int      currentEpoch = 0;

    size_t offset = 0;
    for (int sq = 0; sq < SQUARE_NB; ++sq) {
        Magic& m = magics[sq];
        m.mask  = slider_mask(pt, Square(sq));
        m.shift = 64 - popcount(m.mask);
        m.attacks = pool + offset;

        // Enumerate every subset of the mask (Carry-Rippler) and its attacks.
        Bitboard b = 0;
        int size = 0;
        do {
            occ[size] = b;
            ref[size] = sliding_attack(pt, Square(sq), b);
            ++size;
            b = (b - m.mask) & m.mask;
        } while (b);

        offset += size_t(size);

        // Search for a magic that maps every subset without collision (or with
        // collisions that agree on the same attack set).
        for (;;) {
            Bitboard magic;
            do {
                magic = rng.sparse();
            } while (popcount((m.mask * magic) >> 56) < 6);

            m.magic = magic;
            ++currentEpoch;
            bool ok = true;
            for (int i = 0; i < size; ++i) {
                const unsigned idx = m.index(occ[i]);
                if (epoch[idx] != currentEpoch) {
                    epoch[idx] = currentEpoch;
                    m.attacks[idx] = ref[i];
                } else if (m.attacks[idx] != ref[i]) {
                    ok = false;
                    break;
                }
            }
            if (ok) break;
        }
    }

    if (pt == ROOK)
        assert(offset == 102400);
    else
        assert(offset == 5248);
    (void)offset;
}

bool g_initialized = false;

} // namespace

void init() {
    if (g_initialized) return;
    init_leapers();
    init_magics(ROOK, RookPool, RookMagics);
    init_magics(BISHOP, BishopPool, BishopMagics);
    g_initialized = true;
}

Bitboard pawn(Color c, Square s)   { return PawnTable[c][s]; }
Bitboard knight(Square s)          { return KnightTable[s]; }
Bitboard king(Square s)            { return KingTable[s]; }

Bitboard bishop(Square s, Bitboard occupied) {
    const Magic& m = BishopMagics[s];
    return m.attacks[m.index(occupied)];
}

Bitboard rook(Square s, Bitboard occupied) {
    const Magic& m = RookMagics[s];
    return m.attacks[m.index(occupied)];
}

Bitboard queen(Square s, Bitboard occupied) {
    return bishop(s, occupied) | rook(s, occupied);
}

Bitboard of(PieceType pt, Square s, Bitboard occupied) {
    switch (pt) {
        case KNIGHT: return knight(s);
        case BISHOP: return bishop(s, occupied);
        case ROOK:   return rook(s, occupied);
        case QUEEN:  return queen(s, occupied);
        case KING:   return king(s);
        default:     return 0;
    }
}

} // namespace attacks
} // namespace chess
