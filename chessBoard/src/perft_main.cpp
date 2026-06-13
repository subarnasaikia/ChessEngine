#include "Position.hpp"
#include "Perft.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

using namespace chess;

namespace {

void usage(const char* prog) {
    std::cout <<
        "Usage:\n"
        "  " << prog << " [--divide] <depth> [\"<fen>\"]\n\n"
        "Examples:\n"
        "  " << prog << " 5                       # perft 5 of the start position\n"
        "  " << prog << " --divide 3              # per-move breakdown at depth 3\n"
        "  " << prog << " 4 \"" << STARTPOS_FEN << "\"\n";
}

} // namespace

int main(int argc, char** argv) {
    perft::init();

    bool divide = false;
    int  argi = 1;

    if (argi < argc && std::strcmp(argv[argi], "--divide") == 0) {
        divide = true;
        ++argi;
    }

    if (argi >= argc) {
        usage(argv[0]);
        return 1;
    }

    const int depth = std::atoi(argv[argi++]);
    if (depth < 0) {
        std::cerr << "Depth must be >= 0\n";
        return 1;
    }

    std::string fen = (argi < argc) ? argv[argi] : STARTPOS_FEN;

    Position pos;
    if (!pos.set(fen)) {
        std::cerr << "Invalid FEN: " << fen << '\n';
        return 1;
    }

    std::cout << pos.to_string() << '\n';

    const auto start = std::chrono::steady_clock::now();
    const uint64_t nodes = divide ? perft::divide(pos, depth)
                                  : perft::perft(pos, depth);
    const auto end = std::chrono::steady_clock::now();

    const double secs =
        std::chrono::duration<double>(end - start).count();

    if (!divide)
        std::cout << "perft(" << depth << ") = " << nodes << '\n';

    std::cout << "time: " << secs << "s";
    if (secs > 0.0)
        std::cout << "   (" << uint64_t(nodes / secs) << " nps)";
    std::cout << '\n';

    return 0;
}
