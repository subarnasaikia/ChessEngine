// chessBot tests: evaluation symmetry, and that the search finds mates,
// wins hanging material, always returns a legal move, and is deterministic.

#include "Position.hpp"
#include "Attacks.hpp"
#include "MoveGen.hpp"
#include "Move.hpp"

#include "Evaluation.hpp"
#include "Search.hpp"

#include <iostream>
#include <string>

using namespace chess;

namespace {

int g_fails = 0;

void check(bool cond, const std::string& name) {
    std::cout << (cond ? "[ OK ] " : "[FAIL] ") << name << "\n";
    if (!cond) ++g_fails;
}

chessbot::SearchResult run(const char* fen, int depth) {
    Position pos;
    pos.set(fen);
    chessbot::Search s;
    chessbot::SearchLimits lim;
    lim.maxDepth = depth;
    lim.timeMs   = 0;   // deterministic, depth-only
    return s.search(pos, lim);
}

bool is_legal(const char* fen, Move m) {
    Position pos;
    pos.set(fen);
    MoveList ml;
    movegen::generate_legal(pos, ml);
    for (int i = 0; i < ml.size(); ++i)
        if (ml.moves[i] == m) return true;
    return false;
}

} // namespace

int main() {
    attacks::init();

    const int MATE_THRESHOLD = 29000; // |score| above this means a forced mate

    // --- Mate in 1 -------------------------------------------------------
    {
        const char* fen = "6k1/5ppp/8/8/8/8/8/R6K w - - 0 1"; // Ra8#
        const auto r = run(fen, 4);
        check(to_uci(r.best) == "a1a8", "mate-in-1 (back rank): plays Ra8#");
        check(r.score > MATE_THRESHOLD, "mate-in-1 (back rank): mate score");
    }
    {
        const char* fen = "6k1/5ppp/8/8/8/8/5PPP/4Q1K1 w - - 0 1"; // Qe8#
        const auto r = run(fen, 4);
        check(to_uci(r.best) == "e1e8", "mate-in-1 (queen): plays Qe8#");
        check(r.score > MATE_THRESHOLD, "mate-in-1 (queen): mate score");
    }

    // --- Win material (free queen) ---------------------------------------
    {
        // Black queen on e4 hangs to the knight; Nxe4 wins it outright.
        const char* fen = "rnb1kbnr/pppp1ppp/8/8/4q3/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 1";
        const auto r = run(fen, 5);
        check(to_uci(r.best) == "c3e4", "wins material: captures hanging queen Nxe4");
        check(r.score > 500, "wins material: large positive score");
    }

    // --- Evaluation symmetry ---------------------------------------------
    {
        Position sp;
        sp.set(STARTPOS_FEN);
        check(chessbot::evaluate(sp) == 0, "eval: start position == 0");

        Position a, b; // b is the color-and-rank mirror of a
        a.set("8/8/8/8/8/8/4P3/4K2k w - - 0 1");
        b.set("4k2K/4p3/8/8/8/8/8/8 b - - 0 1");
        check(chessbot::evaluate(a) == chessbot::evaluate(b), "eval: color-mirror symmetry");
    }

    // --- Always returns a legal move -------------------------------------
    {
        const char* fens[] = {
            STARTPOS_FEN,
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        };
        bool ok = true;
        for (const char* f : fens) {
            const auto r = run(f, 4);
            if (!is_legal(f, r.best)) ok = false;
        }
        check(ok, "search returns a legal move");
    }

    // --- Determinism -----------------------------------------------------
    {
        const auto r1 = run(STARTPOS_FEN, 4);
        const auto r2 = run(STARTPOS_FEN, 4);
        check(r1.best == r2.best, "deterministic: same best move on repeated search");
    }

    std::cout << "\n" << (g_fails ? "FAILED" : "All search tests passed.")
              << "  (" << g_fails << " failure(s))\n";
    return g_fails ? 1 : 0;
}
