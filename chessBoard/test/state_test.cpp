// Engine-state tests: Zobrist key integrity, the three draw rules, and the
// Outcome classifier. Complements perft_test (which proves move generation);
// here we prove the bookkeeping that perft does not exercise.

#include "Position.hpp"
#include "MoveGen.hpp"
#include "Outcome.hpp"
#include "Attacks.hpp"

#include <cstdint>
#include <iostream>
#include <string>

using namespace chess;

namespace {

int g_fails = 0;

void check(bool cond, const std::string& name) {
    std::cout << (cond ? "[ OK ] " : "[FAIL] ") << name << "\n";
    if (!cond) ++g_fails;
}

int legal_count(Position& pos) {
    MoveList ml;
    movegen::generate_legal(pos, ml);
    return ml.size();
}

Outcome outcome_of(Position& pos) {
    return classify(pos, legal_count(pos));
}

bool insufficient(const char* fen) {
    Position p;
    p.set(fen);
    return p.is_insufficient_material();
}

// Verify the incrementally-maintained key equals one recomputed from scratch
// (via a FEN round-trip) at every node, and that unmake restores it exactly.
void walk_keys(Position& pos, int depth, int& fails) {
    Position fresh;
    fresh.set(pos.fen());
    if (fresh.key() != pos.key()) { ++fails; return; }
    if (depth == 0) return;

    MoveList ml;
    movegen::generate_legal(pos, ml);
    const int branch = ml.size() < 6 ? ml.size() : 6;
    for (int i = 0; i < branch; ++i) {
        const uint64_t before = pos.key();
        pos.make_move(ml.moves[i]);
        walk_keys(pos, depth - 1, fails);
        pos.unmake_move(ml.moves[i]);
        if (pos.key() != before) ++fails;
    }
}

} // namespace

int main() {
    attacks::init();

    // --- Zobrist key integrity -------------------------------------------
    int keyFails = 0;
    { Position p; p.set(STARTPOS_FEN); walk_keys(p, 4, keyFails); }
    { Position p; p.set("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
      walk_keys(p, 3, keyFails); }
    check(keyFails == 0, "zobrist: incremental key == fresh, restored by unmake");

    // --- Insufficient material -------------------------------------------
    check( insufficient("8/8/8/3k4/8/8/8/4K3 w - - 0 1"),     "insufficient: K vs K");
    check( insufficient("8/8/8/3k4/8/8/8/2B1K3 w - - 0 1"),   "insufficient: KB vs K");
    check( insufficient("8/8/8/3k4/8/8/8/2N1K3 w - - 0 1"),   "insufficient: KN vs K");
    check( insufficient("5b2/8/8/3k4/8/8/8/2B1K3 w - - 0 1"), "insufficient: KB vs KB same color");
    check(!insufficient("b7/8/8/3k4/8/8/8/2B1K3 w - - 0 1"),  "sufficient:  KB vs KB opposite color");
    check(!insufficient("8/8/8/3k4/8/8/8/1N1NK3 w - - 0 1"),  "sufficient:  KNN vs K");
    check(!insufficient("8/8/8/3k4/8/4P3/8/4K3 w - - 0 1"),   "sufficient:  pawn present");

    // --- Fifty-move rule --------------------------------------------------
    { Position p; p.set("8/8/4k3/8/8/4K3/4R3/8 w - - 100 1");
      check(outcome_of(p) == Outcome::DrawFiftyMove, "fifty-move: clock 100 -> draw"); }
    { Position p; p.set("8/8/4k3/8/8/4K3/4R3/8 w - - 99 1");
      check(outcome_of(p) == Outcome::Ongoing, "fifty-move: clock 99 -> ongoing"); }

    // --- Threefold repetition --------------------------------------------
    {
        Position p;
        p.set(STARTPOS_FEN);
        const Move cycle[4] = { Move(G1, F3), Move(G8, F6), Move(F3, G1), Move(F6, G8) };
        for (int rep = 0; rep < 2; ++rep)
            for (Move m : cycle)
                p.make_move(m);
        check(p.repetition_count() == 3, "threefold: startpos reached 3 times");
        check(outcome_of(p) == Outcome::DrawRepetition, "threefold: classify -> draw");
    }

    // --- Checkmate / stalemate via classify ------------------------------
    { Position p; p.set("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
      check(outcome_of(p) == Outcome::Checkmate, "checkmate: fool's mate"); }
    { Position p; p.set("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
      check(outcome_of(p) == Outcome::Stalemate, "stalemate: K+Q vs K"); }

    std::cout << "\n" << (g_fails ? "FAILED" : "All state tests passed.")
              << "  (" << g_fails << " failure(s))\n";
    return g_fails ? 1 : 0;
}
