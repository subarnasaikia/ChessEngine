cmake_minimum_required(VERSION 3.14)

project(chessBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to an optimized build; search is compute-bound.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Engine core. Pulled in automatically by the top-level build; when this module
# is configured on its own, add it directly so the `chessboard` target exists.
if(NOT TARGET chessboard)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../chessBoard
                     ${CMAKE_CURRENT_BINARY_DIR}/chessBoard)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

# Bot library: evaluation + search. Links the engine core.
add_library(chessbot STATIC
    src/Evaluation.cpp
    src/Search.cpp
)
target_include_directories(chessbot PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(chessbot PUBLIC chessboard)

# Headless CLI: prints the chosen move for a FEN.
add_executable(bot src/bot_main.cpp)
target_link_libraries(bot PRIVATE chessbot)

# Search/evaluation tests.
enable_testing()
add_executable(search_test test/search_test.cpp)
target_link_libraries(search_test PRIVATE chessbot)
add_test(NAME bot_suite COMMAND search_test)
