cmake_minimum_required(VERSION 3.14)

project(chessBoard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to an optimized build; perft is compute-bound and benefits greatly.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

include_directories(${PROJECT_SOURCE_DIR}/include)

# Core engine library (everything except executables' main()).
add_library(chessboard STATIC
    src/Bitboard.cpp
    src/Move.cpp
    src/Attacks.cpp
    src/Zobrist.cpp
    src/Position.cpp
    src/MoveGen.cpp
    src/Outcome.cpp
    src/Perft.cpp
)
target_include_directories(chessboard PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Command-line perft tool.
add_executable(perft src/perft_main.cpp)
target_link_libraries(perft PRIVATE chessboard)

# Perft regression test (checks against known reference node counts).
enable_testing()
add_executable(perft_test test/perft_test.cpp)
target_link_libraries(perft_test PRIVATE chessboard)
add_test(NAME perft_suite COMMAND perft_test)

# Engine-state test: Zobrist key integrity, draw rules, outcome classification.
add_executable(state_test test/state_test.cpp)
target_link_libraries(state_test PRIVATE chessboard)
add_test(NAME state_suite COMMAND state_test)
