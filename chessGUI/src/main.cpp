#include <iostream>
#include <array>
#include <memory>
#include "RenderWindow.hpp"
#include "PieceRender.hpp"
#include "Constants.hpp"
#include "errorMessage.hpp"

// Chess engine (chessBoard module): the board state drives what we draw.
#include "Position.hpp"
#include "Attacks.hpp"

namespace {

// Translate an engine piece type into the GUI's PieceType, which is the index
// used into the loaded-texture table.
chessGUI::PieceType to_gui_type(chess::PieceType pt)
{
    switch (pt) {
        case chess::QUEEN:  return chessGUI::PieceType::QUEEN;
        case chess::KING:   return chessGUI::PieceType::KING;
        case chess::ROOK:   return chessGUI::PieceType::ROOK;
        case chess::BISHOP: return chessGUI::PieceType::BISHOP;
        case chess::KNIGHT: return chessGUI::PieceType::KNIGHT;
        case chess::PAWN:   return chessGUI::PieceType::PAWN;
        default:            return chessGUI::PieceType::NO_PIECE;
    }
}

} // namespace

int main()
{
    if(SDL_Init(SDL_INIT_EVERYTHING) > 0)
        errorMessage("SDL_Init has failed.");

    if( !(IMG_Init(IMG_INIT_PNG)))
        errorMessage("IMG_Init has failed.");

    chessGUI::RenderWindow window("Chess", SCREEN_WIDTH, SCREEN_HEIGHT);
    bool gameRunning = true;

    // Load every piece texture once, indexed by [GUI color][GUI piece type].
    std::array<std::array<std::unique_ptr<chessGUI::PieceRender> , 7>, 3> pieces;

    // For white
    pieces[chessGUI::ColorType::WHITE][chessGUI::PieceType::QUEEN]  = std::make_unique<chessGUI::PieceRender>(window, chessGUI::WHITE_QUEEN_PNG,  chessGUI::ColorType::WHITE, chessGUI::PieceType::QUEEN);
    pieces[chessGUI::ColorType::WHITE][chessGUI::PieceType::KING]   = std::make_unique<chessGUI::PieceRender>(window, chessGUI::WHITE_KING_PNG,   chessGUI::ColorType::WHITE, chessGUI::PieceType::KING);
    pieces[chessGUI::ColorType::WHITE][chessGUI::PieceType::ROOK]   = std::make_unique<chessGUI::PieceRender>(window, chessGUI::WHITE_ROOK_PNG,   chessGUI::ColorType::WHITE, chessGUI::PieceType::ROOK);
    pieces[chessGUI::ColorType::WHITE][chessGUI::PieceType::BISHOP] = std::make_unique<chessGUI::PieceRender>(window, chessGUI::WHITE_BISHOP_PNG, chessGUI::ColorType::WHITE, chessGUI::PieceType::BISHOP);
    pieces[chessGUI::ColorType::WHITE][chessGUI::PieceType::KNIGHT] = std::make_unique<chessGUI::PieceRender>(window, chessGUI::WHITE_KNIGHT_PNG, chessGUI::ColorType::WHITE, chessGUI::PieceType::KNIGHT);
    pieces[chessGUI::ColorType::WHITE][chessGUI::PieceType::PAWN]   = std::make_unique<chessGUI::PieceRender>(window, chessGUI::WHITE_PAWN_PNG,   chessGUI::ColorType::WHITE, chessGUI::PieceType::PAWN);

    // For Black
    pieces[chessGUI::ColorType::BLACK][chessGUI::PieceType::QUEEN]  = std::make_unique<chessGUI::PieceRender>(window, chessGUI::BLACK_QUEEN_PNG,  chessGUI::ColorType::BLACK, chessGUI::PieceType::QUEEN);
    pieces[chessGUI::ColorType::BLACK][chessGUI::PieceType::KING]   = std::make_unique<chessGUI::PieceRender>(window, chessGUI::BLACK_KING_PNG,   chessGUI::ColorType::BLACK, chessGUI::PieceType::KING);
    pieces[chessGUI::ColorType::BLACK][chessGUI::PieceType::ROOK]   = std::make_unique<chessGUI::PieceRender>(window, chessGUI::BLACK_ROOK_PNG,   chessGUI::ColorType::BLACK, chessGUI::PieceType::ROOK);
    pieces[chessGUI::ColorType::BLACK][chessGUI::PieceType::BISHOP] = std::make_unique<chessGUI::PieceRender>(window, chessGUI::BLACK_BISHOP_PNG, chessGUI::ColorType::BLACK, chessGUI::PieceType::BISHOP);
    pieces[chessGUI::ColorType::BLACK][chessGUI::PieceType::KNIGHT] = std::make_unique<chessGUI::PieceRender>(window, chessGUI::BLACK_KNIGHT_PNG, chessGUI::ColorType::BLACK, chessGUI::PieceType::KNIGHT);
    pieces[chessGUI::ColorType::BLACK][chessGUI::PieceType::PAWN]   = std::make_unique<chessGUI::PieceRender>(window, chessGUI::BLACK_PAWN_PNG,   chessGUI::ColorType::BLACK, chessGUI::PieceType::PAWN);

    // Engine state. Attack tables must be initialized once before any Position
    // is queried. The board to display comes straight from the engine.
    chess::attacks::init();
    chess::Position pos;
    pos.set(chess::STARTPOS_FEN);

    window.clear();
    window.renderBoard();

    // Draw each occupied square from the engine board. The board is shown with
    // rank 8 at the top (white at the bottom), so screen row = 8 - rank.
    for (int s = 0; s < chess::SQUARE_NB; ++s)
    {
        const chess::Square sq = chess::Square(s);
        const chess::Piece pc = pos.piece_on(sq);
        if (pc == chess::NO_PIECE)
            continue;

        const chessGUI::ColorType guiColor =
            (chess::color_of(pc) == chess::WHITE) ? chessGUI::ColorType::WHITE
                                                  : chessGUI::ColorType::BLACK;
        const chessGUI::PieceType guiType = to_gui_type(chess::type_of(pc));

        const int file = chess::file_of(sq); // 0..7 (a..h)
        const int rank = chess::rank_of(sq); // 0..7 (rank 1..8)
        const int screenCol = file + 1;      // 1..8, left to right
        const int screenRow = 8 - rank;      // rank 8 -> top row 1

        SDL_Rect dst = {
            chessGUI::CENTER_X + (screenCol - 1) * chessGUI::CELL_PIXEL,
            chessGUI::CENTER_Y + (screenRow - 1) * chessGUI::CELL_PIXEL,
            chessGUI::CELL_PIXEL,
            chessGUI::CELL_PIXEL
        };
        window.render(pieces[guiColor][guiType]->getTexture(), NULL, &dst);
    }

    window.display();

    SDL_Event event;
    while(gameRunning)
    {
        while(SDL_WaitEvent((&event)))
        {
            if(event.type == SDL_QUIT)
            {
                gameRunning = false;
                break;
            }
        }
    }
    SDL_Quit();
    return 0;
}
