## Peacekeeper

UCI Chess Engine in Progress

### Compilation

Peacekeeper can be compiled from source with GNU C++, `-O3 -DNDEBUG -march=native` are recommended compiler flags.

I tried to support MSVC intrinsics, but since I do not have it, there might be errors.

### Features

- Board Representation and Move Generation
    - Bitboard-based
    - Redundant Mailbox Board
    - Make-Unmake
    - Fixed-shift Fancy Magic Bitboards for slider move generation
- Search
    - Principal Variation Search (PVS)
    - Quiescence Search
    - Iterative Deepening
    - Staged Move Generation
        - Hash move
        - Captures
        - Quiets
    - Time management
    - Transposition Table
        - Incremental updated Zobrist hash
    - History Table
    - Selectivity
        - Null Move Pruning (NMP)
        - Late Move Reductions (LMR)
        - Late Move Pruning (LMP)
        - Futility Pruning
        - Delta Pruning
    - Move Ordering
        - MVV-LVA for captures
        - History Heuristic and PST for quiet moves
- Evaluation
    - Piece Square Tables (PST)
        - Incrementally updated
    - Tapered Eval
    - Tempo Bonus

### Credits

- The [Chess Programming Wiki](https://www.chessprogramming.org) for being a great resource for everything related to chess programming
- The [Talkchess](talkchess.com) forum and the people on it for answering my more specific questions
- Andrew Zhuo (@StackFish5) for constantly looking over my code
- PeSTO for piece square tables
- Some engines which I got inspiration from (in alphabetical order):
    - [Blunder](https://github.com/algerbrex/blunder)
    - [Leorik](https://github.com/lithander/Leorik)
    - [Stockfish](https://github.com/official-stockfish/Stockfish)
    - [Sunfish](https://github.com/thomasahle/sunfish)
