#include "Bitboard.hpp"

namespace chess {

std::string to_string(Bitboard b) {
    std::string out;
    for (int r = 7; r >= 0; --r) {
        for (int f = 0; f < 8; ++f) {
            out += test_bit(b, make_square(File(f), Rank(r))) ? '1' : '.';
            out += ' ';
        }
        out += '\n';
    }
    return out;
}

} // namespace chess
