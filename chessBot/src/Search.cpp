#include "Search.hpp"
#include "Evaluation.hpp"

#include "MoveGen.hpp"

#include <algorithm>
#include <chrono>

namespace chessbot {

using namespace chess;

namespace {

constexpr int MATE        = 30000;
constexpr int INF         = 32001;
constexpr int MATE_IN_MAX = MATE - 256; // |score| above this is a mate score
constexpr int MAX_PLY     = 128;

constexpr int TT_BITS = 21;             // 2^21 entries (~40 MB)

// Piece values used only for move ordering (MVV-LVA), indexed by PieceType.
const int ORDER_VALUE[7] = { 100, 320, 330, 500, 900, 20000, 0 };

int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

bool is_mate_score(int s) { return s > MATE_IN_MAX || s < -MATE_IN_MAX; }

// Mate scores are stored relative to the root; convert to/from a node's ply so
// a transposed mate keeps the correct distance.
int16_t score_to_tt(int s, int ply) {
    if (s >  MATE_IN_MAX) s += ply;
    else if (s < -MATE_IN_MAX) s -= ply;
    return static_cast<int16_t>(s);
}
int score_from_tt(int s, int ply) {
    if (s >  MATE_IN_MAX) s -= ply;
    else if (s < -MATE_IN_MAX) s += ply;
    return s;
}

} // namespace

Search::Search()
    : tt_(size_t(1) << TT_BITS),
      ttMask_((uint64_t(1) << TT_BITS) - 1),
      nodes_(0), useTime_(false), timeMs_(0), startMs_(0),
      aborted_(false), rootBest_(Move::none()) {}

bool Search::time_up() {
    if (!useTime_) return false;
    return (now_ms() - startMs_) >= timeMs_;
}

void Search::order_moves(const Position& pos, MoveList& ml, Move ttMove) const {
    struct Scored { int s; Move m; };
    Scored scored[256];
    const int n = ml.size();
    for (int i = 0; i < n; ++i) {
        const Move m = ml.moves[i];
        int sc = 0;
        if (ttMove.is_ok() && m == ttMove) {
            sc = 1 << 24;
        } else {
            const PieceType mover = type_of(pos.piece_on(m.from()));
            const bool ep = m.type() == EN_PASSANT;
            const Piece capPc = pos.piece_on(m.to());
            if (capPc != NO_PIECE || ep) {
                const PieceType victim = ep ? PAWN : type_of(capPc);
                sc = (1 << 20) + ORDER_VALUE[victim] * 16 - ORDER_VALUE[mover];
            }
            if (m.type() == PROMOTION)
                sc += (1 << 19) + ORDER_VALUE[m.promotion_type()];
        }
        scored[i] = { sc, m };
    }
    std::stable_sort(scored, scored + n,
                     [](const Scored& a, const Scored& b) { return a.s > b.s; });
    for (int i = 0; i < n; ++i) ml.moves[i] = scored[i].m;
}

int Search::quiescence(Position& pos, int alpha, int beta, int ply) {
    if ((++nodes_ & 2047) == 0 && time_up()) aborted_ = true;
    if (aborted_) return 0;
    if (ply >= MAX_PLY) return evaluate(pos);

    if (ply > 0 && (pos.is_insufficient_material() ||
                    pos.is_fifty_move_rule() ||
                    pos.repetition_count() >= 2))
        return 0;

    // In check: resolve fully (all evasions) so checkmate is seen at the horizon.
    if (pos.in_check()) {
        MoveList ml;
        movegen::generate_legal(pos, ml);
        if (ml.size() == 0) return -MATE + ply;
        order_moves(pos, ml, Move::none());
        int best = -INF;
        for (int i = 0; i < ml.size(); ++i) {
            pos.make_move(ml.moves[i]);
            const int score = -quiescence(pos, -beta, -alpha, ply + 1);
            pos.unmake_move(ml.moves[i]);
            if (aborted_) return 0;
            if (score > best) best = score;
            if (best > alpha) alpha = best;
            if (alpha >= beta) break;
        }
        return best;
    }

    const int standPat = evaluate(pos);
    if (standPat >= beta) return standPat;
    int best = standPat;
    if (standPat > alpha) alpha = standPat;

    MoveList ml;
    movegen::generate_legal(pos, ml);
    order_moves(pos, ml, Move::none());
    for (int i = 0; i < ml.size(); ++i) {
        const Move m = ml.moves[i];
        const bool capture = pos.piece_on(m.to()) != NO_PIECE || m.type() == EN_PASSANT;
        const bool promo   = m.type() == PROMOTION;
        if (!capture && !promo) continue;          // quiet moves: skip in qsearch

        pos.make_move(m);
        const int score = -quiescence(pos, -beta, -alpha, ply + 1);
        pos.unmake_move(m);
        if (aborted_) return 0;
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }
    return best;
}

int Search::negamax(Position& pos, int depth, int alpha, int beta, int ply) {
    if ((++nodes_ & 2047) == 0 && time_up()) aborted_ = true;
    if (aborted_) return 0;

    if (ply > 0 && (pos.is_insufficient_material() ||
                    pos.is_fifty_move_rule() ||
                    pos.repetition_count() >= 2))
        return 0;

    if (depth <= 0)
        return quiescence(pos, alpha, beta, ply);

    // Transposition-table probe (never short-circuits the root).
    Move ttMove = Move::none();
    if (ply > 0) {
        const TTEntry& e = tt_[pos.key() & ttMask_];
        if (e.key == pos.key() && e.bound != BOUND_NONE) {
            ttMove = e.move;
            if (e.depth >= depth) {
                const int s = score_from_tt(e.score, ply);
                if (e.bound == BOUND_EXACT) return s;
                if (e.bound == BOUND_LOWER && s >= beta) return s;
                if (e.bound == BOUND_UPPER && s <= alpha) return s;
            }
        }
    }

    MoveList ml;
    movegen::generate_legal(pos, ml);
    if (ml.size() == 0)
        return pos.in_check() ? (-MATE + ply) : 0;

    order_moves(pos, ml, ttMove);

    const int alphaOrig = alpha;
    int best = -INF;
    Move bestMove = ml.moves[0];
    if (ply == 0) rootBest_ = bestMove;

    for (int i = 0; i < ml.size(); ++i) {
        const Move m = ml.moves[i];
        pos.make_move(m);
        const int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1);
        pos.unmake_move(m);
        if (aborted_) break;
        if (score > best) {
            best = score;
            bestMove = m;
            if (ply == 0) rootBest_ = m;
        }
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }

    if (!aborted_) {
        TTEntry& e = tt_[pos.key() & ttMask_];
        const uint8_t bound = (best <= alphaOrig) ? BOUND_UPPER
                            : (best >= beta)      ? BOUND_LOWER
                                                  : BOUND_EXACT;
        e.key   = pos.key();
        e.move  = bestMove;
        e.score = score_to_tt(best, ply);
        e.depth = static_cast<int16_t>(depth);
        e.bound = bound;
    }

    return best;
}

SearchResult Search::search(Position& pos, const SearchLimits& limits) {
    std::fill(tt_.begin(), tt_.end(), TTEntry{});
    nodes_   = 0;
    useTime_ = limits.timeMs > 0;
    timeMs_  = limits.timeMs;
    startMs_ = now_ms();
    aborted_ = false;
    rootBest_ = Move::none();

    SearchResult res;
    for (int depth = 1; depth <= limits.maxDepth; ++depth) {
        aborted_ = false;
        const int score = negamax(pos, depth, -INF, INF, 0);
        if (aborted_ && depth > 1) break;   // discard the incomplete depth

        res.best  = rootBest_;
        res.score = score;
        res.depth = depth;

        if (is_mate_score(score)) break;    // mate found — deeper won't help
        if (useTime_ && time_up()) break;
    }

    res.nodes     = nodes_;
    res.elapsedMs = static_cast<int>(now_ms() - startMs_);
    return res;
}

} // namespace chessbot
