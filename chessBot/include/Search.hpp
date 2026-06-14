#pragma once

#include "Position.hpp"
#include "Move.hpp"

#include <cstdint>
#include <vector>

namespace chessbot {

// Caps on a single search. timeMs <= 0 means "no time limit" — search to
// exactly maxDepth (deterministic, used by tests).
struct SearchLimits {
    int maxDepth = 64;
    int timeMs   = 1000;
};

struct SearchResult {
    chess::Move best  = chess::Move::none();
    int         score = 0;  // centipawns, side-to-move relative
    int         depth = 0;  // last fully-completed iterative-deepening depth
    uint64_t    nodes = 0;
    int         elapsedMs = 0;
};

// Negamax alpha-beta with iterative deepening, quiescence, MVV-LVA move
// ordering, and a transposition table keyed on the Zobrist hash. One instance
// owns one transposition table; it is safe to reuse across moves of a game.
// Requires attacks::init() to have run before any call.
class Search {
public:
    Search();

    SearchResult search(chess::Position& pos, const SearchLimits& limits);

private:
    enum Bound : uint8_t { BOUND_NONE = 0, BOUND_EXACT, BOUND_LOWER, BOUND_UPPER };

    struct TTEntry {
        uint64_t    key   = 0;
        chess::Move move  = chess::Move::none();
        int16_t     score = 0;
        int16_t     depth = -1;
        uint8_t     bound = BOUND_NONE;
    };

    int negamax(chess::Position& pos, int depth, int alpha, int beta, int ply);
    int quiescence(chess::Position& pos, int alpha, int beta, int ply);

    void order_moves(const chess::Position& pos, chess::MoveList& ml,
                     chess::Move ttMove) const;
    bool time_up();

    std::vector<TTEntry> tt_;
    uint64_t             ttMask_;

    uint64_t nodes_;
    bool     useTime_;
    int      timeMs_;
    int64_t  startMs_;
    bool     aborted_;
    chess::Move rootBest_;
};

} // namespace chessbot
