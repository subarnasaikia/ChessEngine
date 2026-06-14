# ChessEngine

ChessEngine is a modular chess application designed for playing and experimenting with chess. It includes a graphical user interface (GUI), a logic-based chessboard implementation using bitboards, and an evolving chess bot for computer play. 

## Features
- **Chess GUI**: A visually appealing interface for playing chess, built using SDL2 and SDL2_image.
- **Chess Logic**: Implements chess game mechanics using an efficient bitboard representation.
- **Chess Bot**: An AI player (under development) to play against the user.
- **SOLID Principles**: The project design adheres to SOLID principles for maintainable, scalable, and modular code.

---

## Project Structure
The project is organized into the following directories:

### 1. `chessBoard`
Contains the core logic of the chess game, utilizing bitboards for efficient representation and computation of moves. It includes:
- Move validation
- Game state management
- Board representation

### 2. `chessGUI`
Handles the graphical user interface for the chess game. Built using:
- **SDL2**: For rendering the chessboard and pieces.
- **SDL2_image**: For managing images of chess pieces.
- It includes a `CMakeLists.txt` file for building the GUI component.  

### 3. `chessBot`
An AI opponent that plays against the user. Implemented with:
- Negamax **alpha-beta** search with iterative deepening, quiescence, MVV-LVA move ordering, and a transposition table
- **Heuristic evaluation**: material, tapered piece-square tables, mobility, king safety, and pawn structure
- A `bot` CLI (`bot "<fen>" --ms 1000`) and a `bot_suite` test target
- Playable in the GUI (keys `1`/`2`/`3` choose human-vs-human or which side the AI plays)
---

## Design Philosophy
This project adheres to the **SOLID principles** to ensure:
- **S**ingle Responsibility: Each module handles a distinct responsibility (e.g., GUI, logic, AI).
- **O**pen/Closed: Components are designed to allow extension without modifying existing code.
- **L**iskov Substitution: Abstract interfaces are used to enable interchangeability between components.
- **I**nterface Segregation: Components interact through minimal, role-specific interfaces.
- **D**ependency Inversion: High-level modules depend on abstractions, not low-level implementations.

By following these principles, the project aims to remain maintainable, scalable, and testable as it evolves.

---

## Requirements
To build and run ChessEngine, ensure the following dependencies are installed:

### General
- **CMake**: For building the project.

### GUI Requirements
- **SDL2**
- **SDL2_image**
- **SDL2_ttf** (for the on-board result banner)

---

## Building the Project
1. Clone the repository:
   ```bash
   git clone https://github.com/subarnasaikia/ChessEngine.git
   cd ChessEngine
   ```

2. Build the chessGUI module:
   ```bash
   cd chessGUI
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Run the GUI executable:
   ```bash
   ./ChessGUI
   ```

---

## Usage
- Launch the GUI to start a new game.
- Press `1` for human-vs-human, `2` to play White against the AI, or `3` to play Black against the AI. `F5` starts a new game.
- Move by clicking a piece then its destination, or by dragging.

---

## Roadmap
- Run the bot's search on a background thread so the GUI stays responsive while it thinks.
- Strengthen the bot (opening book, deeper/smarter search, evaluation tuning).
- Improve GUI visuals and responsiveness.

---

## License
This project is licensed under the [MIT License](LICENSE).
