// Headless driver for the bot: take a FEN, print the chosen move.
//
//   bot "<fen>" [--ms 1000] [--depth N]
//
// --ms sets the per-move time budget (default 1000). --depth N forces a
// deterministic depth-only search (no time limit), handy for testing.

#include "Position.hpp"
#include "Attacks.hpp"
#include "Move.hpp"
#include "Search.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " \"<fen>\" [--ms 1000] [--depth N]\n";
        return 2;
    }

    chessbot::SearchLimits limits; // defaults: maxDepth 64, timeMs 1000
    const std::string fen = argv[1];

    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--ms") == 0 && i + 1 < argc) {
            limits.timeMs = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--depth") == 0 && i + 1 < argc) {
            limits.maxDepth = std::atoi(argv[++i]);
            limits.timeMs = 0; // depth-only, deterministic
        } else {
            std::cerr << "unknown argument: " << argv[i] << "\n";
            return 2;
        }
    }

    chess::attacks::init();

    chess::Position pos;
    if (!pos.set(fen)) {
        std::cerr << "invalid FEN: " << fen << "\n";
        return 2;
    }

    chessbot::Search search;
    const chessbot::SearchResult r = search.search(pos, limits);

    std::cout << "bestmove " << chess::to_uci(r.best) << "\n";
    std::cout << "info score cp " << r.score
              << " depth " << r.depth
              << " nodes " << r.nodes
              << " time "  << r.elapsedMs << "\n";
    return 0;
}
