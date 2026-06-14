cmake_minimum_required(VERSION 3.14)

project(ChessEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Enable CTest at the top level so the engine's perft suite is discoverable
# via `ctest --test-dir build` from the project root.
enable_testing()

# Engine core (library + perft tool + perft tests). Provides the `chessboard`
# target, whose PUBLIC include directory propagates to anything that links it.
add_subdirectory(chessBoard)

# AI opponent (evaluation + alpha-beta search). Provides the `chessbot` target,
# linked by the GUI. Added before chessGUI so that target exists.
add_subdirectory(chessBot)

# SDL2 GUI. Links the engine and the bot, and renders from a chess::Position.
add_subdirectory(chessGUI)
