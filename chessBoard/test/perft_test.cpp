// Perft verification against the well-known reference node counts from the
// Chess Programming Wiki. If move generation, make/unmake, or legality is
// wrong, these totals diverge immediately — perft is the canonical test.

#include "Position.hpp"
#include "Perft.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace chess;

namespace {

struct Case {
    const char* name;
    std::string fen;
    int         depth;
    uint64_t    expected;
};

// Standard perft suite. Depths are chosen to be thorough but fast (< a few
// seconds total in an optimized build).
const std::vector<Case> kCases = {
    {"startpos", STARTPOS_FEN, 1, 20},
    {"startpos", STARTPOS_FEN, 2, 400},
    {"startpos", STARTPOS_FEN, 3, 8902},
    {"startpos", STARTPOS_FEN, 4, 197281},
    {"startpos", STARTPOS_FEN, 5, 4865609},

    // Kiwipete — dense tactical position exercising castling, ep, promotions.
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},
    {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603},

    // Position 3 — pins, ep, rook/pawn endgame edges.
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14},
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191},
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812},
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238},
    {"position3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624},

    // Position 4 — promotions and many captures (mirror-tested counts).
    {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6},
    {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264},
    {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467},
    {"position4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333},

    // Position 5.
    {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 1, 44},
    {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2, 1486},
    {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
    {"position5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},

    // Position 6.
    {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46},
    {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 2, 2079},
    {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890},
    {"position6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594},
};

} // namespace

int main() {
    perft::init();

    int failures = 0;
    for (const Case& c : kCases) {
        Position pos;
        if (!pos.set(c.fen)) {
            std::cout << "[FAIL] " << c.name << ": invalid FEN\n";
            ++failures;
            continue;
        }

        const uint64_t got = perft::perft(pos, c.depth);
        const bool ok = (got == c.expected);
        if (!ok) ++failures;

        std::cout << (ok ? "[ OK ] " : "[FAIL] ")
                  << c.name << "  perft(" << c.depth << ") = " << got;
        if (!ok)
            std::cout << "  (expected " << c.expected << ")";
        std::cout << '\n';
    }

    std::cout << "\n" << (kCases.size() - failures) << "/" << kCases.size()
              << " perft cases passed.\n";

    if (failures) {
        std::cout << failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "All perft tests passed.\n";
    return 0;
}
