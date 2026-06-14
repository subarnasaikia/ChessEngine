#include "Game.hpp"
#include "Constants.hpp"

#include <string>

// Engine types that have no GUI counterpart can be used unqualified.
using chess::Square;
using chess::Move;
using chess::SQ_NONE;

namespace chessGUI {

namespace {

// Engine piece type -> GUI texture index. Engine enumerators are written
// chess::QUEEN etc. because the GUI namespace defines its own QUEEN/ROOK/...
chessGUI::PieceType to_gui_type(chess::PieceType pt)
{
    switch (pt) {
        case chess::QUEEN:  return chessGUI::QUEEN;
        case chess::KING:   return chessGUI::KING;
        case chess::ROOK:   return chessGUI::ROOK;
        case chess::BISHOP: return chessGUI::BISHOP;
        case chess::KNIGHT: return chessGUI::KNIGHT;
        case chess::PAWN:   return chessGUI::PAWN;
        default:            return chessGUI::NO_PIECE;
    }
}

} // namespace

Game::Game(RenderWindow& window) : _window(window)
{
    // Attack tables must be built once before any Position is queried.
    chess::attacks::init();
    loadTextures();
    reset();
}

void Game::loadTextures()
{
    _pieces[chessGUI::WHITE][chessGUI::QUEEN]  = std::make_unique<PieceRender>(_window, WHITE_QUEEN_PNG,  chessGUI::WHITE, chessGUI::QUEEN);
    _pieces[chessGUI::WHITE][chessGUI::KING]   = std::make_unique<PieceRender>(_window, WHITE_KING_PNG,   chessGUI::WHITE, chessGUI::KING);
    _pieces[chessGUI::WHITE][chessGUI::ROOK]   = std::make_unique<PieceRender>(_window, WHITE_ROOK_PNG,   chessGUI::WHITE, chessGUI::ROOK);
    _pieces[chessGUI::WHITE][chessGUI::BISHOP] = std::make_unique<PieceRender>(_window, WHITE_BISHOP_PNG, chessGUI::WHITE, chessGUI::BISHOP);
    _pieces[chessGUI::WHITE][chessGUI::KNIGHT] = std::make_unique<PieceRender>(_window, WHITE_KNIGHT_PNG, chessGUI::WHITE, chessGUI::KNIGHT);
    _pieces[chessGUI::WHITE][chessGUI::PAWN]   = std::make_unique<PieceRender>(_window, WHITE_PAWN_PNG,   chessGUI::WHITE, chessGUI::PAWN);

    _pieces[chessGUI::BLACK][chessGUI::QUEEN]  = std::make_unique<PieceRender>(_window, BLACK_QUEEN_PNG,  chessGUI::BLACK, chessGUI::QUEEN);
    _pieces[chessGUI::BLACK][chessGUI::KING]   = std::make_unique<PieceRender>(_window, BLACK_KING_PNG,   chessGUI::BLACK, chessGUI::KING);
    _pieces[chessGUI::BLACK][chessGUI::ROOK]   = std::make_unique<PieceRender>(_window, BLACK_ROOK_PNG,   chessGUI::BLACK, chessGUI::ROOK);
    _pieces[chessGUI::BLACK][chessGUI::BISHOP] = std::make_unique<PieceRender>(_window, BLACK_BISHOP_PNG, chessGUI::BLACK, chessGUI::BISHOP);
    _pieces[chessGUI::BLACK][chessGUI::KNIGHT] = std::make_unique<PieceRender>(_window, BLACK_KNIGHT_PNG, chessGUI::BLACK, chessGUI::KNIGHT);
    _pieces[chessGUI::BLACK][chessGUI::PAWN]   = std::make_unique<PieceRender>(_window, BLACK_PAWN_PNG,   chessGUI::BLACK, chessGUI::PAWN);
}

PieceRender* Game::texture(chess::Color c, chess::PieceType pt) const
{
    const chessGUI::ColorType gc = (c == chess::WHITE) ? chessGUI::WHITE : chessGUI::BLACK;
    return _pieces[gc][to_gui_type(pt)].get();
}

void Game::reset()
{
    _pos.set(chess::STARTPOS_FEN);
    _promoting = false;
    _dragging = false;
    clearSelection();
    regenerate();
}

void Game::regenerate()
{
    _legal.count = 0;
    chess::movegen::generate_legal(_pos, _legal);

    _outcome = chess::classify(_pos, _legal.size());

    const chess::Color stm = _pos.side_to_move();
    std::string title = "Chess  -  ";
    if (_outcome == chess::Outcome::Ongoing) {
        title += (stm == chess::WHITE) ? "White to move" : "Black to move";
        if (_pos.in_check()) title += "  (check)";
    } else {
        title += chess::outcome_text(_outcome, stm);
    }
    _window.setTitle(title.c_str());
}

// ---------------------------------------------------------------------------
// Coordinate mapping
// ---------------------------------------------------------------------------
void Game::squareTopLeft(Square sq, int& x, int& y) const
{
    const int file = chess::file_of(sq);   // 0..7, a..h
    const int rank = chess::rank_of(sq);   // 0..7, rank 1..8
    x = CENTER_X + file * CELL_PIXEL;
    y = CENTER_Y + (7 - rank) * CELL_PIXEL;  // rank 8 at the top
}

Square Game::squareAt(int x, int y) const
{
    if (x < CENTER_X || x >= CENTER_X + BOARD_SIZE ||
        y < CENTER_Y || y >= CENTER_Y + BOARD_SIZE)
        return SQ_NONE;

    const int col = (x - CENTER_X) / CELL_PIXEL;  // 0..7 from left
    const int row = (y - CENTER_Y) / CELL_PIXEL;  // 0..7 from top
    return chess::make_square(chess::File(col), chess::Rank(7 - row));
}

// ---------------------------------------------------------------------------
// Move flow
// ---------------------------------------------------------------------------
void Game::clearSelection()
{
    _selected = SQ_NONE;
}

bool Game::hasLegal(Square from, Square to) const
{
    for (Move m : _legal)
        if (m.from() == from && m.to() == to)
            return true;
    return false;
}

void Game::attemptMove(Square from, Square to)
{
    Move matches[4];
    int n = 0;
    for (Move m : _legal)
        if (m.from() == from && m.to() == to && n < 4)
            matches[n++] = m;

    if (n == 0)
        return;                       // illegal: ignore
    if (n == 1) {
        completeMove(matches[0]);     // single concrete move
        return;
    }
    beginPromotion(from, to);         // 4 matches -> choose a promotion piece
}

void Game::completeMove(Move m)
{
    _pos.make_move(m);
    _dragging = false;
    clearSelection();
    regenerate();
}

void Game::beginPromotion(Square from, Square to)
{
    _promoting = true;
    _promoFrom = from;
    _promoTo   = to;
    _dragging  = false;
    clearSelection();
}

void Game::choosePromotion(chess::PieceType pt)
{
    if (!_promoting)
        return;
    for (Move m : _legal) {
        if (m.from() == _promoFrom && m.to() == _promoTo &&
            m.type() == chess::PROMOTION && m.promotion_type() == pt) {
            _promoting = false;
            _pos.make_move(m);
            regenerate();
            return;
        }
    }
}

void Game::promotionCellRect(int index, int& x, int& y) const
{
    int bx, by;
    squareTopLeft(_promoTo, bx, by);
    // White promotes on the top rank (cells stack downward); black on the
    // bottom rank (cells stack upward).
    const int dir = (_pos.side_to_move() == chess::WHITE) ? 1 : -1;
    x = bx;
    y = by + index * dir * CELL_PIXEL;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void Game::onMouseDown(int x, int y)
{
    _mouseX = x;
    _mouseY = y;

    if (_promoting) {
        const chess::PieceType choices[4] =
            { chess::QUEEN, chess::ROOK, chess::BISHOP, chess::KNIGHT };
        for (int i = 0; i < 4; ++i) {
            int cx, cy;
            promotionCellRect(i, cx, cy);
            if (x >= cx && x < cx + CELL_PIXEL && y >= cy && y < cy + CELL_PIXEL) {
                choosePromotion(choices[i]);
                return;
            }
        }
        _promoting = false;   // clicked off the picker: cancel
        clearSelection();
        return;
    }

    if (_outcome != chess::Outcome::Ongoing)
        return;

    const Square sq = squareAt(x, y);
    if (sq == SQ_NONE) {
        clearSelection();
        return;
    }

    // Second click of a click-click move onto a highlighted legal target.
    if (_selected != SQ_NONE && hasLegal(_selected, sq)) {
        attemptMove(_selected, sq);
        return;
    }

    // Otherwise: select a friendly piece and arm a potential drag.
    const chess::Piece pc = _pos.piece_on(sq);
    if (pc != chess::NO_PIECE && chess::color_of(pc) == _pos.side_to_move()) {
        _selected = sq;
        _dragging = true;
        _dragFrom = sq;
    } else {
        clearSelection();
    }
}

void Game::onMouseUp(int x, int y)
{
    _mouseX = x;
    _mouseY = y;
    if (!_dragging)
        return;
    _dragging = false;

    const Square sq = squareAt(x, y);
    if (sq != SQ_NONE && sq != _dragFrom && hasLegal(_dragFrom, sq))
        attemptMove(_dragFrom, sq);
    // Released on the origin (a click) or an illegal target: the piece stays
    // selected so a following click can complete the move.
}

void Game::onKey(SDL_Keycode key)
{
    if (_promoting) {
        switch (key) {
            case SDLK_q: choosePromotion(chess::QUEEN);  break;
            case SDLK_r: choosePromotion(chess::ROOK);   break;
            case SDLK_b: choosePromotion(chess::BISHOP); break;
            case SDLK_n: choosePromotion(chess::KNIGHT); break;
            case SDLK_ESCAPE: _promoting = false; clearSelection(); break;
            default: break;
        }
        return;
    }
    switch (key) {
        case SDLK_ESCAPE: clearSelection(); _dragging = false; break;
        case SDLK_F5:     reset(); break;   // new game
        default: break;
    }
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
void Game::renderHighlights()
{
    // King in check.
    if (_outcome == chess::Outcome::Ongoing && _pos.in_check()) {
        int x, y;
        squareTopLeft(_pos.king_square(_pos.side_to_move()), x, y);
        _window.fillRect(x, y, CELL_PIXEL, CELL_PIXEL, 220, 50, 50, 140);
    }

    if (_selected == SQ_NONE)
        return;

    int sx, sy;
    squareTopLeft(_selected, sx, sy);
    _window.fillRect(sx, sy, CELL_PIXEL, CELL_PIXEL, 246, 246, 130, 150);

    for (Move m : _legal) {
        if (m.from() != _selected)
            continue;
        const Square to = m.to();
        int x, y;
        squareTopLeft(to, x, y);
        const bool capture =
            (_pos.piece_on(to) != chess::NO_PIECE) || (m.type() == chess::EN_PASSANT);
        if (capture)
            _window.fillRect(x, y, CELL_PIXEL, CELL_PIXEL, 210, 70, 70, 95);
        else
            _window.fillCircle(x + CELL_PIXEL / 2, y + CELL_PIXEL / 2,
                               CELL_PIXEL / 7, 40, 40, 40, 110);
    }
}

void Game::renderPieces()
{
    for (int s = 0; s < chess::SQUARE_NB; ++s) {
        const Square sq = Square(s);
        if (_dragging && sq == _dragFrom)
            continue;                          // drawn under the cursor instead
        const chess::Piece pc = _pos.piece_on(sq);
        if (pc == chess::NO_PIECE)
            continue;
        int x, y;
        squareTopLeft(sq, x, y);
        SDL_Rect dst = { x, y, CELL_PIXEL, CELL_PIXEL };
        _window.render(texture(chess::color_of(pc), chess::type_of(pc))->getTexture(), NULL, &dst);
    }
}

void Game::renderPromotionPicker()
{
    const chess::Color us = _pos.side_to_move();
    const chess::PieceType choices[4] =
        { chess::QUEEN, chess::ROOK, chess::BISHOP, chess::KNIGHT };
    for (int i = 0; i < 4; ++i) {
        int x, y;
        promotionCellRect(i, x, y);
        _window.fillRect(x, y, CELL_PIXEL, CELL_PIXEL, 235, 235, 235, 255);
        // thin frame
        _window.fillRect(x, y, CELL_PIXEL, 2, 120, 120, 120, 255);
        _window.fillRect(x, y + CELL_PIXEL - 2, CELL_PIXEL, 2, 120, 120, 120, 255);
        _window.fillRect(x, y, 2, CELL_PIXEL, 120, 120, 120, 255);
        _window.fillRect(x + CELL_PIXEL - 2, y, 2, CELL_PIXEL, 120, 120, 120, 255);
        SDL_Rect dst = { x, y, CELL_PIXEL, CELL_PIXEL };
        _window.render(texture(us, choices[i])->getTexture(), NULL, &dst);
    }
}

void Game::renderGameOverBanner()
{
    // Dim the whole board so the result reads clearly.
    _window.fillRect(CENTER_X, CENTER_Y, BOARD_SIZE, BOARD_SIZE, 0, 0, 0, 150);

    // Centered panel with a light frame.
    const int panelW = BOARD_SIZE - 80;
    const int panelH = 96;
    const int px = CENTER_X + (BOARD_SIZE - panelW) / 2;
    const int py = CENTER_Y + (BOARD_SIZE - panelH) / 2;
    _window.fillRect(px, py, panelW, panelH, 28, 28, 32, 235);
    _window.fillRect(px, py, panelW, 3, 235, 235, 235, 255);
    _window.fillRect(px, py + panelH - 3, panelW, 3, 235, 235, 235, 255);

    const int cx = CENTER_X + BOARD_SIZE / 2;
    const SDL_Color result = { 245, 245, 245, 255 };
    const SDL_Color hint   = { 175, 175, 180, 255 };
    _window.drawTextCentered(chess::outcome_text(_outcome, _pos.side_to_move()),
                             cx, py + panelH / 2 - 16, result);
    _window.drawTextCentered("Press F5 for a new game",
                             cx, py + panelH / 2 + 18, hint);
}

void Game::render()
{
    _window.clear();
    _window.renderBoard();
    renderHighlights();
    renderPieces();

    if (_dragging && _dragFrom != SQ_NONE) {
        const chess::Piece pc = _pos.piece_on(_dragFrom);
        if (pc != chess::NO_PIECE) {
            SDL_Rect dst = { _mouseX - CELL_PIXEL / 2, _mouseY - CELL_PIXEL / 2,
                             CELL_PIXEL, CELL_PIXEL };
            _window.render(texture(chess::color_of(pc), chess::type_of(pc))->getTexture(), NULL, &dst);
        }
    }

    if (_promoting)
        renderPromotionPicker();

    if (_outcome != chess::Outcome::Ongoing)
        renderGameOverBanner();

    _window.display();
}

// ---------------------------------------------------------------------------
// Event loop
// ---------------------------------------------------------------------------
void Game::run()
{
    render();

    SDL_Event e;
    while (_running) {
        if (SDL_WaitEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    _running = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT)
                        onMouseDown(e.button.x, e.button.y);
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT)
                        onMouseUp(e.button.x, e.button.y);
                    break;
                case SDL_MOUSEMOTION:
                    _mouseX = e.motion.x;
                    _mouseY = e.motion.y;
                    break;
                case SDL_KEYDOWN:
                    onKey(e.key.keysym.sym);
                    break;
                default:
                    break;
            }
            render();
        }
    }
}

} // namespace chessGUI
