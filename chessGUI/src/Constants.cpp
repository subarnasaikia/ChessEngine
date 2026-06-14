#include "Constants.hpp"

// The build system defines CHESS_RES_DIR as an absolute path to chessGUI/res,
// so piece assets load regardless of the current working directory. The
// fallback keeps ad-hoc compiles (run from a build/ dir) working as before.
#ifndef CHESS_RES_DIR
#define CHESS_RES_DIR "../res"
#endif

namespace chessGUI {
    const char* WHITE_KING_PNG   = CHESS_RES_DIR "/white-king.png";
    const char* WHITE_QUEEN_PNG  = CHESS_RES_DIR "/white-queen.png";
    const char* WHITE_ROOK_PNG   = CHESS_RES_DIR "/white-rook.png";
    const char* WHITE_BISHOP_PNG = CHESS_RES_DIR "/white-bishop.png";
    const char* WHITE_KNIGHT_PNG = CHESS_RES_DIR "/white-knight.png";
    const char* WHITE_PAWN_PNG   = CHESS_RES_DIR "/white-pawn.png";

    const char* BLACK_KING_PNG   = CHESS_RES_DIR "/black-king.png";
    const char* BLACK_QUEEN_PNG  = CHESS_RES_DIR "/black-queen.png";
    const char* BLACK_ROOK_PNG   = CHESS_RES_DIR "/black-rook.png";
    const char* BLACK_BISHOP_PNG = CHESS_RES_DIR "/black-bishop.png";
    const char* BLACK_KNIGHT_PNG = CHESS_RES_DIR "/black-knight.png";
    const char* BLACK_PAWN_PNG   = CHESS_RES_DIR "/black-pawn.png";

    const char* FONT_TTF         = CHESS_RES_DIR "/DejaVuSans.ttf";

    const int CELL_PIXEL = BOARD_SIZE / 8;
    const int CENTER_X = (SCREEN_WIDTH - BOARD_SIZE) / 2;
    const int CENTER_Y = (SCREEN_HEIGHT - BOARD_SIZE) / 2;
}
