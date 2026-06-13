#include "Perft.hpp"
#include "MoveGen.hpp"
#include "Attacks.hpp"

#include <iostream>

namespace chess {
namespace perft {

void init() {
    attacks::init();
}

uint64_t perft(Position& pos, int depth) {
    if (depth == 0)
        return 1;

    MoveList list;
    movegen::generate(pos, list);

    const Color us = pos.side_to_move();
    uint64_t nodes = 0;

    for (Move m : list) {
        pos.make_move(m);
        // Legal iff the side that just moved did not leave its king in check.
        if (!pos.is_attacked(pos.king_square(us), ~us))
            nodes += (depth == 1) ? 1 : perft(pos, depth - 1);
        pos.unmake_move(m);
    }
    return nodes;
}

uint64_t divide(Position& pos, int depth) {
    MoveList list;
    movegen::generate(pos, list);

    const Color us = pos.side_to_move();
    uint64_t total = 0;

    for (Move m : list) {
        pos.make_move(m);
        if (!pos.is_attacked(pos.king_square(us), ~us)) {
            const uint64_t n = (depth <= 1) ? 1 : perft(pos, depth - 1);
            total += n;
            std::cout << to_uci(m) << ": " << n << '\n';
        }
        pos.unmake_move(m);
    }
    std::cout << "\nNodes searched: " << total << '\n';
    return total;
}

} // namespace perft
} // namespace chess
