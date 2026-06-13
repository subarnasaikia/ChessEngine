#pragma once

#include <array>
#include <memory>

#include "RenderWindow.hpp"
#include "PieceRender.hpp"

#include "Position.hpp"
#include "MoveGen.hpp"

namespace chessGUI {

// Drives an interactive chess game on top of the chessBoard engine: owns the
// position, the legal moves for the side to move, and the transient input
// state (selection, drag, pending promotion), and renders everything.
//
// Moving a piece works two ways, unified through the same legality check:
//   - click-click: click a piece (legal targets light up), then click a target
//   - drag-and-drop: press a piece, drag, release on a target
class Game {
public:
    explicit Game(RenderWindow& window);

    // Blocking event loop. Returns when the window is closed.
    void run();

private:
    enum class Status { Ongoing, Checkmate, Stalemate };

    // Setup
    void loadTextures();
    void reset();                 // new game from the start position
    void regenerate();            // recompute legal moves, status, window title

    // Input
    void onMouseDown(int x, int y);
    void onMouseUp(int x, int y);
    void onKey(SDL_Keycode key);

    // Move flow
    bool hasLegal(chess::Square from, chess::Square to) const;
    void attemptMove(chess::Square from, chess::Square to);  // makes it, or opens promotion picker
    void completeMove(chess::Move m);
    void beginPromotion(chess::Square from, chess::Square to);
    void choosePromotion(chess::PieceType pt);
    void promotionCellRect(int index, int& x, int& y) const;  // index 0..3 -> Q,R,B,N
    void clearSelection();

    // Rendering
    void render();
    void renderHighlights();
    void renderPieces();
    void renderPromotionPicker();

    // Coordinate mapping (board is drawn with rank 8 at the top)
    chess::Square squareAt(int x, int y) const;        // pixel -> square (SQ_NONE if off-board)
    void squareTopLeft(chess::Square sq, int& x, int& y) const;
    PieceRender* texture(chess::Color c, chess::PieceType pt) const;

    RenderWindow& _window;
    std::array<std::array<std::unique_ptr<PieceRender>, 7>, 3> _pieces;

    chess::Position _pos;
    chess::MoveList _legal;
    Status _status = Status::Ongoing;

    chess::Square _selected = chess::SQ_NONE;

    bool          _dragging = false;
    chess::Square _dragFrom = chess::SQ_NONE;
    int           _mouseX = 0;
    int           _mouseY = 0;

    bool          _promoting = false;
    chess::Square _promoFrom = chess::SQ_NONE;
    chess::Square _promoTo   = chess::SQ_NONE;

    bool _running = true;
};

} // namespace chessGUI
