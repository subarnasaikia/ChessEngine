# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ChessEngine is a modular C++ chess application. Each module is its own CMake project, built and developed independently.

| Module | Status | What it is |
|--------|--------|------------|
| `chessBoard` | Working core | Bitboard engine: move generation, make/unmake, perft. No search/eval yet. |
| `chessGUI` | Partial | SDL2 renderer. Draws the start position **from the engine** (`chess::Position`); no input or move-making yet. |
| `chessBot` | Not present | AI opponent named in the root README roadmap; the directory does not exist yet. |

Keep this table honest as the project grows. Treat a module as "Working" only when something verifiable backs it (e.g. `chessBoard`'s perft suite). Document what exists in code, not roadmap intentions.

## Build & test

The top-level `CMakeLists.txt` builds everything — the engine library, `perft`, `perft_test`, and the GUI:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure    # perft regression suite
./build/chessGUI/chessGUI                      # run the GUI (needs a display)
```
Each module can also be configured standalone from its own directory (the GUI's CMake
pulls in the engine automatically when `chessboard` isn't already a target).

### chessBoard (engine)
Requires CMake + a C++17 compiler. No third-party dependencies.
```
cd chessBoard
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure    # perft regression suite
```
- Run perft directly: `./build/perft 5` or `./build/perft --divide 3 "<fen>"`.
- `perft_test` runs the whole reference suite; there is no finer test target. To check a single position, run the `perft` CLI with that FEN and depth.
- No-CMake fallback (compile the lib sources + a `main` with `c++ -std=c++17 -O2 -Iinclude ...`) is documented in `chessBoard/README.md`.

### chessGUI (renderer)
Requires CMake, SDL2, and SDL2_image (found via `pkg-config`).
```
cd chessGUI && mkdir build && cd build && cmake .. && make
./chessGUI
```
- Piece assets resolve via an absolute path baked in at compile time (`CHESS_RES_DIR`, set by `chessGUI/CMakeLists.txt` to `chessGUI/res`), so the executable runs from any directory. `chessGUI/src/Constants.cpp` falls back to `../res` if the define is absent.

## Architecture

### chessBoard — the engine
A layered pipeline. To understand it, read the headers in this order (`chessBoard/include`):
`Types.hpp` → `Bitboard.hpp` → `Attacks.hpp` → `Position.hpp` → `MoveGen.hpp` → `Perft.hpp`.

Load-bearing invariants:
- **Initialize before use.** `attacks::init()` (or `perft::init()`, which calls it) fills the leaper tables and *searches for the magic numbers at runtime* using a fixed PRNG seed. It MUST run once before any attack lookup or any `Position`. Results are deterministic across runs.
- **Square mapping is LERF**: `square = rank*8 + file`, A1 = 0 … H8 = 63 (`Types.hpp`). Every bitboard, mask, and shift assumes this.
- **Sliding attacks use magic bitboards** (`Attacks.cpp`): relevant-occupancy mask → magic multiply → table lookup. Pool sizes (102400 rook, 5248 bishop) are asserted during init.
- **Legality = pseudo-legal + make/unmake.** `movegen::generate` emits pseudo-legal moves (may leave the own king in check). The caller makes the move, tests `is_attacked(king_square(mover), ~mover)`, then unmakes; `Position` holds a `StateInfo` undo stack. **Exception:** castling is fully validated inside the generator (the king may not start in, pass through, or land on an attacked square), because the pass-through square is not covered by a post-move king-safety test.
- **Moves are packed into 16 bits** (`Move.hpp`): to, from, promotion type, and a 2-bit flag (normal / promotion / en passant / castling).
- **Correctness is proven by perft, not by asserting on internals.** `test/perft_test.cpp` compares node counts against the Chess Programming Wiki reference positions (startpos, Kiwipete, positions 3–6). After any change to move generation or make/unmake, re-run ctest — a divergent perft count is a bug.

### chessGUI — the renderer
SDL2. `main.cpp` creates a `RenderWindow`, loads the 12 piece textures into `PieceRender` objects, draws the checkered board, then iterates a `chess::Position` and blits each piece onto its square. The event loop only handles `SDL_QUIT`: the board is drawn once from engine state and is not yet interactive.

### Cross-module
The top-level `CMakeLists.txt` builds both modules and `chessGUI` links the `chessboard` library. `main.cpp` builds a `chess::Position`, calls `chess::attacks::init()`, and draws each square from `piece_on(...)` — rendering is engine-driven, not hard-coded. It maps engine `(color, type)` to the GUI texture table and engine rank/file to screen coordinates (rank 8 on top, so `screenRow = 8 - rank`). Still unbuilt: mouse input and move-making — the GUI can display any position but cannot change it.

## Conventions & gotchas
- **Filename case matters.** `Bitboard.hpp/.cpp` use a lowercase `b`. macOS is case-insensitive, so a mismatched-case `#include` compiles locally but breaks on case-sensitive Linux/CI. Match the on-disk case exactly.
- The root `README.md` states a SOLID/modular design intent. There is currently no lint, formatter, or CI configuration in the repo.
