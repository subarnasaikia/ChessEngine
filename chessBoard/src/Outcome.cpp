#include "Outcome.hpp"

namespace chess {

Outcome classify(const Position& pos, int legalMoveCount) {
    if (legalMoveCount == 0)
        return pos.in_check() ? Outcome::Checkmate : Outcome::Stalemate;

    // A true dead position takes precedence; the other draws are order-neutral.
    if (pos.is_insufficient_material())
        return Outcome::DrawInsufficientMaterial;
    if (pos.repetition_count() >= 3)
        return Outcome::DrawRepetition;
    if (pos.is_fifty_move_rule())
        return Outcome::DrawFiftyMove;

    return Outcome::Ongoing;
}

const char* outcome_text(Outcome o, Color sideToMove) {
    switch (o) {
        case Outcome::Checkmate:
            return sideToMove == WHITE ? "Checkmate - Black wins"
                                       : "Checkmate - White wins";
        case Outcome::Stalemate:                return "Draw - stalemate";
        case Outcome::DrawFiftyMove:            return "Draw - fifty-move rule";
        case Outcome::DrawRepetition:           return "Draw - threefold repetition";
        case Outcome::DrawInsufficientMaterial: return "Draw - insufficient material";
        case Outcome::Ongoing:
        default:                                return "";
    }
}

} // namespace chess
