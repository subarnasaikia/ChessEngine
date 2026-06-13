#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "RenderWindow.hpp"
#include "Constants.hpp"
#include "errorMessage.hpp"
#include "Game.hpp"

int main()
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        errorMessage("SDL_Init has failed.");

    if( !(IMG_Init(IMG_INIT_PNG)))
        errorMessage("IMG_Init has failed.");

    chessGUI::RenderWindow window("Chess", SCREEN_WIDTH, SCREEN_HEIGHT);

    chessGUI::Game game(window);
    game.run();

    IMG_Quit();
    SDL_Quit();
    return 0;
}
