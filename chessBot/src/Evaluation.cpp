#include "Evaluation.hpp"

#include "Bitboard.hpp"
#include "Attacks.hpp"

namespace chessbot {

using namespace chess;

namespace {

// --- Material (PeSTO midgame/endgame base values), indexed by PieceType ----
const int MG_VALUE[6] = { 82, 337, 365, 477, 1025, 0 };
const int EG_VALUE[6] = { 94, 281, 297, 512,  936, 0 };

// Game-phase weight per piece type; full midgame = 24.
const int PHASE_INC[6] = { 0, 1, 1, 2, 4, 0 };

// --- Piece-square tables (PeSTO), White's view, index 0 = a8 … 63 = h1 ------
// A White piece on square `sq` reads table[sq ^ 56]; Black reads table[sq].
const int MG_PST[6][64] = {
    { // Pawn
          0,   0,   0,   0,   0,   0,   0,   0,
         98, 134,  61,  95,  68, 126,  34, -11,
         -6,   7,  26,  31,  65,  56,  25, -20,
        -14,  13,   6,  21,  23,  12,  17, -23,
        -27,  -2,  -5,  12,  17,   6,  10, -25,
        -26,  -4,  -4, -10,   3,   3,  33, -12,
        -35,  -1, -20, -23, -15,  24,  38, -22,
          0,   0,   0,   0,   0,   0,   0,   0 },
    { // Knight
       -167, -89, -34, -49,  61, -97, -15,-107,
        -73, -41,  72,  36,  23,  62,   7, -17,
        -47,  60,  37,  65,  84, 129,  73,  44,
         -9,  17,  19,  53,  37,  69,  18,  22,
        -13,   4,  16,  13,  28,  19,  21,  -8,
        -23,  -9,  12,  10,  19,  17,  25, -16,
        -29, -53, -12,  -3,  -1,  18, -14, -19,
       -105, -21, -58, -33, -17, -28, -19, -23 },
    { // Bishop
        -29,   4, -82, -37, -25, -42,   7,  -8,
        -26,  16, -18, -13,  30,  59,  18, -47,
        -16,  37,  43,  40,  35,  50,  37,  -2,
         -4,   5,  19,  50,  37,  37,   7,  -2,
         -6,  13,  13,  26,  34,  12,  10,   4,
          0,  15,  15,  15,  14,  27,  18,  10,
          4,  15,  16,   0,   7,  21,  33,   1,
        -33,  -3, -14, -21, -13, -12, -39, -21 },
    { // Rook
         32,  42,  32,  51,  63,   9,  31,  43,
         27,  32,  58,  62,  80,  67,  26,  44,
         -5,  19,  26,  36,  17,  45,  61,  16,
        -24, -11,   7,  26,  24,  35,  -8, -20,
        -36, -26, -12,  -1,   9,  -7,   6, -23,
        -45, -25, -16, -17,   3,   0,  -5, -33,
        -44, -16, -20,  -9,  -1,  11,  -6, -71,
        -19, -13,   1,  17,  16,   7, -37, -26 },
    { // Queen
        -28,   0,  29,  12,  59,  44,  43,  45,
        -24, -39,  -5,   1, -16,  57,  28,  54,
        -13, -17,   7,   8,  29,  56,  47,  57,
        -27, -27, -16, -16,  -1,  17,  -2,   1,
         -9, -26,  -9, -10,  -2,  -4,   3,  -3,
        -14,   2, -11,  -2,  -5,   2,  14,   5,
        -35,  -8,  11,   2,   8,  15,  -3,   1,
         -1, -18,  -9,  10, -15, -25, -31, -50 },
    { // King
        -65,  23,  16, -15, -56, -34,   2,  13,
         29,  -1, -20,  -7,  -8,  -4, -38, -29,
         -9,  24,   2, -16, -20,   6,  22, -22,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -49,  -1, -27, -39, -46, -44, -33, -51,
        -14, -14, -22, -46, -44, -30, -15, -27,
          1,   7,  -8, -64, -43, -16,   9,   8,
        -15,  36,  12, -54,   8, -28,  24,  14 }
};

const int EG_PST[6][64] = {
    { // Pawn
          0,   0,   0,   0,   0,   0,   0,   0,
        178, 173, 158, 134, 147, 132, 165, 187,
         94, 100,  85,  67,  56,  53,  82,  84,
         32,  24,  13,   5,  -2,   4,  17,  17,
         13,   9,  -3,  -7,  -7,  -8,   3,  -1,
          4,   7,  -6,   1,   0,  -5,  -1,  -8,
         13,   8,   8,  10,  13,   0,   2,  -7,
          0,   0,   0,   0,   0,   0,   0,   0 },
    { // Knight
        -58, -38, -13, -28, -31, -27, -63, -99,
        -25,  -8, -25,  -2,  -9, -25, -24, -52,
        -24, -20,  10,   9,  -1,  -9, -19, -41,
        -17,   3,  22,  22,  22,  11,   8, -18,
        -18,  -6,  16,  25,  16,  17,   4, -18,
        -23,  -3,  -1,  15,  10,  -3, -20, -22,
        -42, -20, -10,  -5,  -2, -20, -23, -44,
        -29, -51, -23, -15, -22, -18, -50, -64 },
    { // Bishop
        -14, -21, -11,  -8,  -7,  -9, -17, -24,
         -8,  -4,   7, -12,  -3, -13,  -4, -14,
          2,  -8,   0,  -1,  -2,   6,   0,   4,
         -3,   9,  12,   9,  14,  10,   3,   2,
         -6,   3,  13,  19,   7,  10,  -3,  -9,
        -12,  -3,   8,  10,  13,   3,  -7, -15,
        -14, -18,  -7,  -1,   4,  -9, -15, -27,
        -23,  -9, -23,  -5,  -9, -16,  -5, -17 },
    { // Rook
         13,  10,  18,  15,  12,  12,   8,   5,
         11,  13,  13,  11,  -3,   3,   8,   3,
          7,   7,   7,   5,   4,  -3,  -5,  -3,
          4,   3,  13,   1,   2,   1,  -1,   2,
          3,   5,   8,   4,  -5,  -6,  -8, -11,
         -4,   0,  -5,  -1,  -7, -12,  -8, -16,
         -6,  -6,   0,   2,  -9,  -9, -11,  -3,
         -9,   2,   3,  -1,  -5, -13,   4, -20 },
    { // Queen
         -9,  22,  22,  27,  27,  19,  10,  20,
        -17,  20,  32,  41,  58,  25,  30,   0,
        -20,   6,   9,  49,  47,  35,  19,   9,
          3,  22,  24,  45,  57,  40,  57,  36,
        -18,  28,  19,  47,  31,  34,  39,  23,
        -16, -27,  15,   6,   9,  17,  10,   5,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -33, -28, -22, -43,  -5, -32, -20, -41 },
    { // King
        -74, -35, -18, -18, -11,  15,   4, -17,
        -12,  17,  14,  17,  17,  38,  23,  11,
         10,  17,  23,  15,  20,  45,  44,  13,
         -8,  22,  24,  27,  26,  33,  26,   3,
        -18,  -4,  21,  24,  27,  23,   9, -11,
        -19,  -3,  11,  21,  23,  16,   7,  -9,
        -27, -11,   4,  13,  14,   4,  -5, -17,
        -53, -34, -21, -11, -28, -14, -24, -43 }
};

// Tuning weights for the non-PeSTO terms (centipawns).
const int MOBILITY_WEIGHT = 2;
const int SHELTER_OPEN     = 12; // penalty for an open file beside the king
const int SHELTER_PAWN     = 8;  // bonus for a friendly pawn shielding a file
const int DOUBLED_PENALTY  = 16;
const int ISOLATED_PENALTY = 14;
const int PASSED_BONUS[8]  = { 0, 8, 12, 20, 36, 64, 100, 0 }; // by rank from own side

inline Bitboard adjacent_files(int f) {
    Bitboard m = 0;
    if (f > 0) m |= file_bb(File(f - 1));
    if (f < 7) m |= file_bb(File(f + 1));
    return m;
}

// Mobility: number of squares friendly sliders/knights attack that are not
// occupied by friendly pieces.
int mobility(const Position& pos, Color c) {
    const Bitboard occ = pos.pieces();
    const Bitboard own = pos.pieces(c);
    int m = 0;
    for (PieceType pt : { KNIGHT, BISHOP, ROOK, QUEEN }) {
        Bitboard bb = pos.pieces(c, pt);
        while (bb) {
            const Square s = pop_lsb(bb);
            m += popcount(attacks::of(pt, s, occ) & ~own);
        }
    }
    return m;
}

// King shelter: friendly pawns on the king's file and the two adjacent files
// shelter it; open files beside the king are penalized. Midgame concern only.
int king_shelter(const Position& pos, Color c) {
    const Square ksq = pos.king_square(c);
    const int kf = file_of(ksq);
    const Bitboard ownPawns = pos.pieces(c, PAWN);
    int s = 0;
    for (int df = -1; df <= 1; ++df) {
        const int f = kf + df;
        if (f < 0 || f > 7) continue;
        if (ownPawns & file_bb(File(f))) s += SHELTER_PAWN;
        else                             s -= SHELTER_OPEN;
    }
    return s;
}

// Pawn structure for one color; the caller subtracts Black's from White's.
int pawn_structure(const Position& pos, Color c) {
    const Bitboard ownPawns = pos.pieces(c, PAWN);
    const Bitboard enemyPawns = pos.pieces(~c, PAWN);
    int score = 0;

    // Doubled pawns: every extra pawn on a file is penalized.
    for (int f = 0; f < 8; ++f) {
        const int n = popcount(ownPawns & file_bb(File(f)));
        if (n > 1) score -= (n - 1) * DOUBLED_PENALTY;
    }

    Bitboard bb = ownPawns;
    while (bb) {
        const Square s = pop_lsb(bb);
        const int f = file_of(s);
        const int r = rank_of(s);

        // Isolated: no friendly pawn on either adjacent file.
        if (!(ownPawns & adjacent_files(f)))
            score -= ISOLATED_PENALTY;

        // Passed: no enemy pawn on this or adjacent files ahead of it.
        const Bitboard frontFiles = file_bb(File(f)) | adjacent_files(f);
        Bitboard ahead;
        if (c == WHITE)
            ahead = ~Bitboard(0) << ((r + 1) * 8);          // ranks above r
        else
            ahead = (r == 0) ? 0 : (~Bitboard(0) >> ((8 - r) * 8)); // ranks below r
        if (!(enemyPawns & frontFiles & ahead)) {
            const int adv = (c == WHITE) ? r : (7 - r);
            score += PASSED_BONUS[adv];
        }
    }
    return score;
}

} // namespace

int evaluate(const Position& pos) {
    int mg[2] = { 0, 0 };
    int eg[2] = { 0, 0 };
    int phase = 0;

    for (int sq = 0; sq < SQUARE_NB; ++sq) {
        const Piece piece = pos.piece_on(Square(sq));
        if (piece == NO_PIECE) continue;
        const Color c  = color_of(piece);
        const PieceType pt = type_of(piece);
        const int idx = (c == WHITE) ? (sq ^ 56) : sq; // PeSTO orientation
        mg[c] += MG_VALUE[pt] + MG_PST[pt][idx];
        eg[c] += EG_VALUE[pt] + EG_PST[pt][idx];
        phase += PHASE_INC[pt];
    }

    const int mgScore = mg[WHITE] - mg[BLACK];
    const int egScore = eg[WHITE] - eg[BLACK];

    const int mgPhase = phase > 24 ? 24 : phase;
    const int egPhase = 24 - mgPhase;
    int score = (mgScore * mgPhase + egScore * egPhase) / 24; // White-relative

    // Mobility (untapered, small weight).
    score += MOBILITY_WEIGHT * (mobility(pos, WHITE) - mobility(pos, BLACK));

    // King shelter scaled toward the midgame.
    score += (king_shelter(pos, WHITE) - king_shelter(pos, BLACK)) * mgPhase / 24;

    // Pawn structure.
    score += pawn_structure(pos, WHITE) - pawn_structure(pos, BLACK);

    return (pos.side_to_move() == WHITE) ? score : -score;
}

} // namespace chessbot
