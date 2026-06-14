#pragma once

namespace chessGUI {
    #define SCREEN_WIDTH 940
    #define SCREEN_HEIGHT 720
    #define BOARD_SIZE 620

    extern const int CELL_PIXEL;
    extern const int CENTER_X;
    extern const int CENTER_Y;

    extern const char* WHITE_KING_PNG;
    extern const char* WHITE_QUEEN_PNG;
    extern const char* WHITE_ROOK_PNG;
    extern const char* WHITE_BISHOP_PNG;
    extern const char* WHITE_KNIGHT_PNG;
    extern const char* WHITE_PAWN_PNG;

    extern const char* BLACK_KING_PNG;
    extern const char* BLACK_QUEEN_PNG;
    extern const char* BLACK_ROOK_PNG;
    extern const char* BLACK_BISHOP_PNG;
    extern const char* BLACK_KNIGHT_PNG;
    extern const char* BLACK_PAWN_PNG;

    // Font used for the game-over banner (bundled in res/, resolved via CHESS_RES_DIR).
    extern const char* FONT_TTF;
}
