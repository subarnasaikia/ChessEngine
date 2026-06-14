# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

ChessEngine is a modular C++ chess application. Each module is its own CMake project, built and developed independently.

| Module | Status | What it is |
|--------|--------|------------|
| `chessBoard` | Working core | Bitboard engine: move generation, make/unmake, perft, Zobrist keys, end-of-game classification. No search/eval (that lives in `chessBot`). |
| `chessBot` | Working | AI opponent: heuristic evaluation + negamax alpha-beta search (iterative deepening, quiescence, MVV-LVA ordering, transposition table). Static lib `chessbot` + `bot` CLI; backed by the `bot_suite` tests. |
| `chessGUI` | Playable | SDL2 GUI driven by the engine: click-or-drag to move (legal moves only, highlighted), in-window promotion picker, turn alternation, full end-of-game detection shown in the title and an on-board banner, and human-vs-AI play (keys `1`/`2`/`3`). |

Keep this table honest as the project grows. Treat a module as "Working" only when something verifiable backs it (e.g. `chessBoard`'s perft suite). Document what exists in code, not roadmap intentions.

## Build & test

The top-level `CMakeLists.txt` builds everything — the engine library, `perft`,
the bot library + `bot` CLI, all test suites, and the GUI:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure    # perft_suite + state_suite + bot_suite
./build/chessGUI/chessGUI                      # run the GUI (needs a display)
./build/chessBot/bot "<fen>" --ms 1000         # ask the bot for a move
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
- `ctest` runs two suites: `perft_suite` (`perft_test`, the move-generation reference counts) and `state_suite` (`state_test`, Zobrist-key integrity, the draw rules, and `classify`). To check a single position, run the `perft` CLI with that FEN and depth.
- No-CMake fallback (compile the lib sources + a `main` with `c++ -std=c++17 -O2 -Iinclude ...`) is documented in `chessBoard/README.md`.

### chessGUI (renderer)
Requires CMake, SDL2, SDL2_image, and SDL2_ttf (all found via `pkg-config`; on
macOS, `brew install sdl2_ttf`).
```
cd chessGUI && mkdir build && cd build && cmake .. && make
./chessGUI
```
- Piece assets and the banner font (`DejaVuSans.ttf`, bundled in `chessGUI/res`; see `DejaVuSans.LICENSE.txt`) resolve via an absolute path baked in at compile time (`CHESS_RES_DIR`, set by `chessGUI/CMakeLists.txt` to `chessGUI/res`), so the executable runs from any directory. `chessGUI/src/Constants.cpp` falls back to `../res` if the define is absent. A missing font is non-fatal — the banner panel still draws, just without text.

### chessBot (AI)
Requires CMake + a C++17 compiler. No third-party dependencies (links `chessboard`).
```
cd chessBot
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure    # bot_suite
./build/bot "<fen>" --ms 1000                  # or --depth N for a fixed-depth, deterministic search
```
- `--ms N` is the per-move time budget; `--depth N` forces a deterministic depth-only search (used by tests).

## Architecture

### chessBoard — the engine
A layered pipeline. To understand it, read the headers in this order (`chessBoard/include`):
`Types.hpp` → `Bitboard.hpp` → `Attacks.hpp` → `Zobrist.hpp` → `Position.hpp` → `MoveGen.hpp` → `Outcome.hpp` → `Perft.hpp`.

Load-bearing invariants:
- **Initialize before use.** `attacks::init()` (or `perft::init()`, which calls it) fills the leaper tables and *searches for the magic numbers at runtime* using a fixed PRNG seed. It MUST run once before any attack lookup or any `Position`. Results are deterministic across runs.
- **Square mapping is LERF**: `square = rank*8 + file`, A1 = 0 … H8 = 63 (`Types.hpp`). Every bitboard, mask, and shift assumes this.
- **Sliding attacks use magic bitboards** (`Attacks.cpp`): relevant-occupancy mask → magic multiply → table lookup. Pool sizes (102400 rook, 5248 bishop) are asserted during init.
- **Legality = pseudo-legal + make/unmake.** `movegen::generate` emits pseudo-legal moves (may leave the own king in check). The caller makes the move, tests `is_attacked(king_square(mover), ~mover)`, then unmakes; `Position` holds a `StateInfo` undo stack. **Exception:** castling is fully validated inside the generator (the king may not start in, pass through, or land on an attacked square), because the pass-through square is not covered by a post-move king-safety test. `movegen::generate_legal` packages this filter and returns only legal moves.
- **Game outcome lives in the engine, not the GUI.** `chess::classify(pos, legalMoveCount)` (`Outcome.hpp`) is the single source of truth for checkmate, stalemate, and the three draws; it takes the caller's already-generated legal-move count so it does not re-run move generation. `outcome_text()` formats the result string shared by the title and the GUI banner.
- **`Position` carries an incremental Zobrist key** (`Zobrist.hpp`). The piece-square term rides along inside `put_piece`/`remove_piece`/`move_piece`; `make_move`/`unmake_move` handle the side/castling/ep terms and a per-ply key stack used by `repetition_count()` (threefold). `zobrist::init()` is idempotent and run by `Position::set`. Draw predicates `is_insufficient_material()` and `is_fifty_move_rule()` are pure `Position` queries.
- **Moves are packed into 16 bits** (`Move.hpp`): to, from, promotion type, and a 2-bit flag (normal / promotion / en passant / castling).
- **Correctness is proven by perft, not by asserting on internals.** `test/perft_test.cpp` compares node counts against the Chess Programming Wiki reference positions (startpos, Kiwipete, positions 3–6). After any change to move generation or make/unmake, re-run ctest — a divergent perft count is a bug.

### chessBot — the AI
Two units in `chessBot` (`include`/`src`), layered on the engine:
- **`Evaluation.hpp`** — `evaluate(pos)` returns centipawns from the side-to-move's perspective (negamax-friendly: a position and its color-mirror are negatives). Terms: material + tapered PeSTO piece-square tables (blended midgame/endgame by a phase from remaining material), mobility, king shelter, and pawn structure (doubled/isolated/passed).
- **`Search.hpp`** — `Search::search(pos, SearchLimits)` returns a `SearchResult{best, score, depth, nodes}`. Negamax alpha-beta with iterative deepening (time-limited via `SearchLimits{maxDepth, timeMs}`; `timeMs<=0` = depth-only), quiescence (with check-evasion so mates are seen at the horizon), MVV-LVA move ordering, and a Zobrist-keyed transposition table (one `Search` owns one TT, cleared each call). Mate scores are ply-adjusted so faster mates win; in-search draws use `repetition_count() >= 2` (the first repetition).
- **Assumes legal positions.** Like the engine, search/eval assume the side to move cannot capture the enemy king. Feeding an illegal FEN (opponent already in check) to the `bot` CLI can crash via a kingless position — positions reached through legal play are always fine.

### chessGUI — the renderer
SDL2. `main.cpp` is thin: it initializes SDL and hands off to the `Game` class (`chessGUI/include/Game.hpp`), which owns the `chess::Position`, the current legal moves (`movegen::generate_legal`), and the input state (selection, drag, pending promotion). A blocking `SDL_WaitEvent` loop re-renders after each event. Moving works two ways through one legality check — **click-click** and **drag-and-drop**; the selected square, legal targets, and a checked king are drawn as translucent overlays (`RenderWindow::fillRect`/`fillCircle`); promotion shows an in-window Q/R/B/N picker (click, or press q/r/b/n). `F5` starts a new game (same mode), `Esc` cancels a selection. The window title reports whose move it is and check; once `chess::classify` reports a terminal `Outcome`, input is locked, the title shows the result, and a translucent on-board banner (drawn with `RenderWindow::drawTextCentered`, SDL2_ttf) names it. Keys `1`/`2`/`3` switch mode (human-vs-human, you-White-vs-AI, AI-White-vs-you). In an AI mode, `Game::maybePlayAI()` runs `chessbot::Search` synchronously after the human's move (and on a new game when the AI has White), showing an "AI thinking…" overlay; the window is briefly frozen for the ~1 s budget — threading the search is a noted future enhancement.

### Cross-module
The top-level `CMakeLists.txt` builds all three modules; `chessBot` links `chessboard`, and `chessGUI` links both `chessbot` and `chessboard`. The GUI's `Game` drives a `chess::Position` directly: it maps engine `(color, type)` to the GUI texture table and engine rank/file to screen coordinates (rank 8 on top, so `screenRow = 8 - rank`), enforces legality via `movegen::generate_legal`, and (in an AI mode) asks `chessbot::Search` for the opponent's move. All three core pieces — engine, bot, GUI — are now in place.

## Conventions & gotchas
- **Filename case matters.** `Bitboard.hpp/.cpp` use a lowercase `b`. macOS is case-insensitive, so a mismatched-case `#include` compiles locally but breaks on case-sensitive Linux/CI. Match the on-disk case exactly.
- The root `README.md` states a SOLID/modular design intent. There is currently no lint, formatter, or CI configuration in the repo.
