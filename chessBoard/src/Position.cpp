#include "Position.hpp"
#include "Zobrist.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace chess {

namespace {

// Map a FEN piece letter to a Piece (uppercase = white, lowercase = black).
Piece piece_from_char(char c) {
    switch (c) {
        case 'P': return W_PAWN;   case 'N': return W_KNIGHT;
        case 'B': return W_BISHOP; case 'R': return W_ROOK;
        case 'Q': return W_QUEEN;  case 'K': return W_KING;
        case 'p': return B_PAWN;   case 'n': return B_KNIGHT;
        case 'b': return B_BISHOP; case 'r': return B_ROOK;
        case 'q': return B_QUEEN;  case 'k': return B_KING;
        default:  return NO_PIECE;
    }
}

char char_from_piece(Piece pc) {
    static const char w[] = {'P', 'N', 'B', 'R', 'Q', 'K'};
    static const char b[] = {'p', 'n', 'b', 'r', 'q', 'k'};
    return color_of(pc) == WHITE ? w[type_of(pc)] : b[type_of(pc)];
}

// Castling-rights mask for a square: rights to clear when a piece leaves or
// lands on this square (king start, rook starts, and rook capture squares).
int cr_mask(Square s) {
    switch (s) {
        case E1: return ANY_CASTLING & ~(WHITE_OO | WHITE_OOO);
        case A1: return ANY_CASTLING & ~WHITE_OOO;
        case H1: return ANY_CASTLING & ~WHITE_OO;
        case E8: return ANY_CASTLING & ~(BLACK_OO | BLACK_OOO);
        case A8: return ANY_CASTLING & ~BLACK_OOO;
        case H8: return ANY_CASTLING & ~BLACK_OO;
        default: return ANY_CASTLING;
    }
}

// For a castling king destination, the rook's from/to squares.
void castling_rook_squares(Square kingTo, Square& rFrom, Square& rTo) {
    switch (kingTo) {
        case G1: rFrom = H1; rTo = F1; break; // white short
        case C1: rFrom = A1; rTo = D1; break; // white long
        case G8: rFrom = H8; rTo = F8; break; // black short
        case C8: rFrom = A8; rTo = D8; break; // black long
        default: rFrom = rTo = SQ_NONE; break;
    }
}

} // namespace

void Position::clear() {
    for (int i = 0; i < PIECE_TYPE_NB; ++i) byType_[i] = 0;
    byColor_[WHITE] = byColor_[BLACK] = 0;
    for (int s = 0; s < SQUARE_NB; ++s) board_[s] = NO_PIECE;
    sideToMove_ = WHITE;
    castling_ = NO_CASTLING;
    epSquare_ = SQ_NONE;
    halfmoveClock_ = 0;
    fullmoveNumber_ = 1;
    key_ = 0;
    repKeys_.clear();
    history_.clear();
}

// The three mutators below also fold the piece-square term into the Zobrist
// key, so make/unmake never have to hash pieces explicitly. They assume
// zobrist::init() has run (Position::set guarantees it).
void Position::put_piece(Piece pc, Square s) {
    const Bitboard b = square_bb(s);
    board_[s] = pc;
    byType_[type_of(pc)] |= b;
    byColor_[color_of(pc)] |= b;
    key_ ^= zobrist::psq[pc][s];
}

void Position::remove_piece(Square s) {
    const Piece pc = board_[s];
    const Bitboard b = square_bb(s);
    byType_[type_of(pc)] ^= b;
    byColor_[color_of(pc)] ^= b;
    board_[s] = NO_PIECE;
    key_ ^= zobrist::psq[pc][s];
}

void Position::move_piece(Square from, Square to) {
    const Piece pc = board_[from];
    const Bitboard fromTo = square_bb(from) ^ square_bb(to);
    byType_[type_of(pc)] ^= fromTo;
    byColor_[color_of(pc)] ^= fromTo;
    board_[from] = NO_PIECE;
    board_[to] = pc;
    key_ ^= zobrist::psq[pc][from] ^ zobrist::psq[pc][to];
}

bool Position::set(const std::string& fen) {
    clear();
    zobrist::init();    // psq tables must exist before put_piece hashes pieces
    std::istringstream ss(fen);
    std::string placement, stm, castle, ep;
    if (!(ss >> placement >> stm >> castle >> ep))
        return false;

    // Piece placement, from rank 8 down to rank 1.
    int rank = 7, file = 0;
    for (char c : placement) {
        if (c == '/') {
            --rank;
            file = 0;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            file += c - '0';
        } else {
            const Piece pc = piece_from_char(c);
            if (pc == NO_PIECE || rank < 0 || file > 7) return false;
            put_piece(pc, make_square(File(file), Rank(rank)));
            ++file;
        }
    }

    sideToMove_ = (stm == "w") ? WHITE : BLACK;

    castling_ = NO_CASTLING;
    for (char c : castle) {
        switch (c) {
            case 'K': castling_ |= WHITE_OO;  break;
            case 'Q': castling_ |= WHITE_OOO; break;
            case 'k': castling_ |= BLACK_OO;  break;
            case 'q': castling_ |= BLACK_OOO; break;
            case '-': default: break;
        }
    }

    if (ep != "-" && ep.size() == 2) {
        const File f = File(ep[0] - 'a');
        const Rank r = Rank(ep[1] - '1');
        epSquare_ = make_square(f, r);
    } else {
        epSquare_ = SQ_NONE;
    }

    // Halfmove and fullmove counters are optional.
    int hm = 0, fm = 1;
    if (ss >> hm) halfmoveClock_ = hm;
    if (ss >> fm) fullmoveNumber_ = fm;

    // Piece-square terms were accumulated by put_piece during placement; add
    // the side-to-move, castling, and en-passant terms to complete the key.
    if (sideToMove_ == BLACK) key_ ^= zobrist::side;
    key_ ^= zobrist::castling[castling_];
    if (epSquare_ != SQ_NONE) key_ ^= zobrist::epFile[file_of(epSquare_)];

    repKeys_.push_back(key_);
    return true;
}

std::string Position::fen() const {
    std::ostringstream ss;
    for (int r = 7; r >= 0; --r) {
        int run = 0;
        for (int f = 0; f < 8; ++f) {
            const Piece pc = board_[make_square(File(f), Rank(r))];
            if (pc == NO_PIECE) {
                ++run;
            } else {
                if (run) { ss << run; run = 0; }
                ss << char_from_piece(pc);
            }
        }
        if (run) ss << run;
        if (r) ss << '/';
    }

    ss << ' ' << (sideToMove_ == WHITE ? 'w' : 'b') << ' ';

    if (castling_ == NO_CASTLING) {
        ss << '-';
    } else {
        if (castling_ & WHITE_OO)  ss << 'K';
        if (castling_ & WHITE_OOO) ss << 'Q';
        if (castling_ & BLACK_OO)  ss << 'k';
        if (castling_ & BLACK_OOO) ss << 'q';
    }

    ss << ' ';
    if (epSquare_ == SQ_NONE) {
        ss << '-';
    } else {
        ss << char('a' + file_of(epSquare_)) << char('1' + rank_of(epSquare_));
    }

    ss << ' ' << halfmoveClock_ << ' ' << fullmoveNumber_;
    return ss.str();
}

Bitboard Position::attackers_to(Square s, Bitboard occ) const {
    return (attacks::pawn(BLACK, s) & pieces(WHITE, PAWN))
         | (attacks::pawn(WHITE, s) & pieces(BLACK, PAWN))
         | (attacks::knight(s)      & pieces(KNIGHT))
         | (attacks::king(s)        & pieces(KING))
         | (attacks::bishop(s, occ) & pieces(BISHOP, QUEEN))
         | (attacks::rook(s, occ)   & pieces(ROOK, QUEEN));
}

bool Position::is_attacked(Square s, Color by) const {
    return attackers_to(s, pieces()) & pieces(by);
}

void Position::make_move(Move m) {
    StateInfo st;
    st.castling      = castling_;
    st.epSquare      = epSquare_;
    st.halfmoveClock = halfmoveClock_;
    st.captured      = NO_PIECE_TYPE;
    st.key           = key_;

    const Color us = sideToMove_;
    const Color them = ~us;
    const Square from = m.from();
    const Square to = m.to();
    const MoveType mt = m.type();
    const PieceType pt = type_of(board_[from]);

    // Drop the previous en-passant file from the key before clearing it.
    if (epSquare_ != SQ_NONE) key_ ^= zobrist::epFile[file_of(epSquare_)];
    epSquare_ = SQ_NONE;
    ++halfmoveClock_;

    if (mt == CASTLING) {
        Square rFrom, rTo;
        castling_rook_squares(to, rFrom, rTo);
        move_piece(from, to);   // king
        move_piece(rFrom, rTo); // rook
    } else if (mt == EN_PASSANT) {
        const Square capSq = (us == WHITE) ? Square(to - 8) : Square(to + 8);
        st.captured = PAWN;
        remove_piece(capSq);
        move_piece(from, to);
        halfmoveClock_ = 0;
    } else {
        if (board_[to] != NO_PIECE) {
            st.captured = type_of(board_[to]);
            remove_piece(to);
            halfmoveClock_ = 0;
        }
        move_piece(from, to);

        if (pt == PAWN) {
            halfmoveClock_ = 0;
            if (mt == PROMOTION) {
                remove_piece(to); // remove the pawn
                put_piece(make_piece(us, m.promotion_type()), to);
            } else if ((int(from) ^ int(to)) == 16) {
                epSquare_ = Square((from + to) / 2); // double push
            }
        }
    }

    // Castling-rights change: swap the old mask term for the new one.
    key_ ^= zobrist::castling[castling_];
    castling_ &= cr_mask(from) & cr_mask(to);
    key_ ^= zobrist::castling[castling_];

    // A new en-passant target (double pawn push) enters the key.
    if (epSquare_ != SQ_NONE) key_ ^= zobrist::epFile[file_of(epSquare_)];

    if (us == BLACK) ++fullmoveNumber_;
    sideToMove_ = them;
    key_ ^= zobrist::side;

    history_.push_back(st);
    repKeys_.push_back(key_);
}

void Position::unmake_move(Move m) {
    const StateInfo st = history_.back();
    history_.pop_back();

    const Color us = ~sideToMove_; // the side that made the move
    const Color them = sideToMove_;
    if (us == BLACK) --fullmoveNumber_;
    sideToMove_ = us;

    const Square from = m.from();
    const Square to = m.to();
    const MoveType mt = m.type();

    if (mt == CASTLING) {
        Square rFrom, rTo;
        castling_rook_squares(to, rFrom, rTo);
        move_piece(to, from);   // king back
        move_piece(rTo, rFrom); // rook back
    } else {
        if (mt == PROMOTION) {
            remove_piece(to);                       // remove promoted piece
            put_piece(make_piece(us, PAWN), to);    // restore pawn at 'to'
        }
        move_piece(to, from);                       // move piece back

        if (mt == EN_PASSANT) {
            const Square capSq = (us == WHITE) ? Square(to - 8) : Square(to + 8);
            put_piece(make_piece(them, PAWN), capSq);
        } else if (st.captured != NO_PIECE_TYPE) {
            put_piece(make_piece(them, st.captured), to);
        }
    }

    castling_      = st.castling;
    epSquare_      = st.epSquare;
    halfmoveClock_ = st.halfmoveClock;
    // The piece mutators above also touched key_; discard that and restore the
    // exact pre-move key, then drop this ply from the repetition history.
    key_           = st.key;
    repKeys_.pop_back();
}

int Position::repetition_count() const {
    const int n = static_cast<int>(repKeys_.size());
    // repKeys_.back() is the current position. Only positions reached since the
    // last irreversible move (capture/pawn move, which reset halfmoveClock_) can
    // repeat the current one, so scanning that window is exact and sufficient.
    const int window = std::min(halfmoveClock_, n - 1);
    int count = 0;
    for (int i = 0; i <= window; ++i)
        if (repKeys_[n - 1 - i] == key_)
            ++count;
    return count;
}

bool Position::is_insufficient_material() const {
    // A pawn, rook, or queen always permits checkmate.
    if (pieces(PAWN) | pieces(ROOK) | pieces(QUEEN))
        return false;

    const Bitboard knights = pieces(KNIGHT);
    const Bitboard bishops = pieces(BISHOP);
    const int minors = popcount(knights) + popcount(bishops);

    if (minors <= 1)
        return true;            // K vs K, or king + a single minor vs king
    if (knights)
        return false;           // a knight beside any other minor can mate

    // Bishops only: a dead position iff every bishop sits on one color complex.
    const Bitboard dark = 0xAA55AA55AA55AA55ULL;
    return !(bishops & dark) || !(bishops & ~dark);
}

std::string Position::to_string() const {
    std::ostringstream ss;
    ss << "  +-----------------+\n";
    for (int r = 7; r >= 0; --r) {
        ss << (r + 1) << " | ";
        for (int f = 0; f < 8; ++f) {
            const Piece pc = board_[make_square(File(f), Rank(r))];
            ss << (pc == NO_PIECE ? '.' : char_from_piece(pc)) << ' ';
        }
        ss << "|\n";
    }
    ss << "  +-----------------+\n";
    ss << "    a b c d e f g h\n";
    ss << "FEN: " << fen() << '\n';
    return ss.str();
}

} // namespace chess
