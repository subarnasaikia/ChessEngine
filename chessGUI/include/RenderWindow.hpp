#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace chessGUI {
    class RenderWindow {
        public:
            RenderWindow(const char* w_title, int w_width, int w_height);
            SDL_Texture* loadTexture(const char* p_filePath);
            SDL_Texture* loadImage(const char* p_filePath);
            void render(SDL_Texture* p_tex , SDL_Rect* src, SDL_Rect* dst);

            void renderBoard();

            // Alpha-blended overlay primitives (used for move highlights).
            void fillRect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
            void fillCircle(int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

            void setTitle(const char* title);

            void clear();
            void display();
            ~RenderWindow();

        private:
            SDL_Window* _window;
            SDL_Renderer* _renderer;
    };

}