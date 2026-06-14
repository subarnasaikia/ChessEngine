#pragma once

#include "Position.hpp"

namespace chess {

// The terminal state of a game, or Ongoing. Single source of truth for
// checkmate, stalemate, and the three automatic draws, shared by the GUI and
// any future engine consumer.
enum class Outcome {
    Ongoing,
    Checkmate,
    Stalemate,
    DrawFiftyMove,
    DrawRepetition,
    DrawInsufficientMaterial
};

// Classify `pos` given the number of legal moves the caller has already
// generated for the side to move (so this does not re-run move generation).
//
//   legalMoveCount == 0  ->  Checkmate (in check) or Stalemate.
//   otherwise            ->  the first applicable draw, else Ongoing.
//
// Mate/stalemate take priority over the draw rules: a mate delivered on the
// fifty-move boundary is still a mate.
Outcome classify(const Position& pos, int legalMoveCount);

// Human-readable result, e.g. "Checkmate - White wins",
// "Draw - insufficient material". `sideToMove` names the checkmate winner
// (the side delivering mate is the side NOT to move). Empty string for Ongoing.
const char* outcome_text(Outcome o, Color sideToMove);

} // namespace chess
