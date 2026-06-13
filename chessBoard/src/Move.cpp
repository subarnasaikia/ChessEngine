#include "Move.hpp"

namespace chess {

namespace {
std::string square_name(Square s) {
    std::string n;
    n += char('a' + file_of(s));
    n += char('1' + rank_of(s));
    return n;
}
} // namespace

std::string to_uci(Move m) {
    if (!m.is_ok())
        return "0000";

    std::string s = square_name(m.from()) + square_name(m.to());
    if (m.type() == PROMOTION) {
        const char promo[] = {'n', 'b', 'r', 'q'};
        s += promo[m.promotion_type() - KNIGHT];
    }
    return s;
}

} // namespace chess
