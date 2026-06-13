# chessBoard

The core chess logic for ChessEngine: a bitboard board representation, magic-bitboard
move generation, make/unmake, and a `perft` harness that verifies correctness against
known reference node counts.

## Design

- **Board representation** — Little-Endian Rank-File (LERF) bitboards. One 64-bit word
  per piece type and per color, plus a `Piece` array for fast square lookups.
- **Sliding attacks** — fancy **magic bitboards**. Magics are found at startup with a
  fixed-seed PRNG, so the tables are deterministic and reproducible.
- **Legality** — move generation is **pseudo-legal**; perft filters illegal moves via
  `make_move` + a king-safety test, then `unmake_move`. Castling is fully validated at
  generation time (king may not start in, pass through, or land on an attacked square).
- **Moves** — packed into 16 bits (from, to, promotion type, and a flag for
  normal / promotion / en passant / castling).

Covered rules: pawn pushes/double-pushes/captures, en passant, all four promotions,
castling rights and legality, check detection.

## Files

| File | Purpose |
|------|---------|
| `include/Types.hpp`      | Colors, pieces, squares, files/ranks, castling, directions |
| `include/Bitboard.hpp`   | `Bitboard` ops: popcount, bit scans, wrap-safe shifts |
| `include/Move.hpp`       | 16-bit move encoding + `MoveList` |
| `include/Attacks.hpp`    | Attack-table interface (leapers + magic sliders) |
| `include/Position.hpp`   | Board state, FEN I/O, make/unmake, attack queries |
| `include/MoveGen.hpp`    | Pseudo-legal move generation |
| `include/Perft.hpp`      | Perft driver |
| `src/*.cpp`              | Implementations |
| `src/perft_main.cpp`     | `perft` command-line tool |
| `test/perft_test.cpp`    | Reference-count regression suite |

## Build & run

### With CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure   # run the perft suite
./build/perft 5                               # perft 5 of the start position
./build/perft --divide 3                      # per-move breakdown
./build/perft 4 "<fen>"                       # perft of any FEN
```

### Without CMake (direct compile)

```bash
c++ -std=c++17 -O2 -Iinclude \
    src/Bitboard.cpp src/Move.cpp src/Attacks.cpp \
    src/Position.cpp src/MoveGen.cpp src/Perft.cpp \
    test/perft_test.cpp -o perft_test && ./perft_test
```

## Verified perft results

All pass (`26/26`), matching the Chess Programming Wiki reference values:

| Position | Depth | Nodes |
|----------|-------|-------|
| Start position | 5 | 4,865,609 |
| Start position | 6 | 119,060,324 |
| Kiwipete | 4 | 4,085,603 |
| Position 3 | 5 | 674,624 |
| Position 4 | 4 | 422,333 |
| Position 5 | 4 | 2,103,487 |
| Position 6 | 4 | 3,894,594 |

## Not yet included

Search / evaluation (the `chessBot`), Zobrist hashing, and GUI integration. Perft is
the correctness foundation those build on.
