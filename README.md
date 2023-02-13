## Peacekeeper

Yet another UCI Chess Engine in Progress written in C++17.

### Compilation

Peacekeeper can be compiled from source with GNU C++, `-O3 -DNDEBUG -march=native` are recommended compiler flags.

I tried to support MSVC intrinsics, but since I do not have it, there might be errors.

Windows and linux executables are also provided.

### Ratings

Version | CCRL Blitz | CCRL 40/15 | MCERL
--------|------------|------------|------
v1.10   | 2421       | 2385       | 2248

### Features

- Board Representation and Move Generation
    - Bitboard-based
    - Redundant Mailbox Board
    - Make-Unmake
    - Fixed-shift Fancy Magic Bitboards for slider move generation
    - Legal move generation
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
        - Incrementally updated Zobrist hash
    - Selectivity
        - Check Extensions
        - Static Null Move Pruning/Reverse Futility Pruning (SNMP/RFP)
        - Null Move Pruning (NMP)
        - Late Move Reductions (LMR)
        - Futility Pruning
        - Delta Pruning
    - Move Ordering
        - MVV-LVA for captures
        - History Heuristic for quiet moves
- Evaluation
    - Texel Tuned
    - Piece Square Tables (PST)
        - Incrementally updated
    - Tapered Eval
    - Tempo Bonus

### Credits & Thanks

- Pradu Kannan for magic multipliers
- CPW (and Sungorus) for zobrist hash pseudorandom number generator
- The [Chess Programming Wiki](https://www.chessprogramming.org) for being a great resource for everything related to chess programming
- The [Talkchess](https://talkchess.com) forum and the people on it for answering my more specific questions
- Andrew Zhuo (@StackFish5) for constantly looking over my code
- PeSTO for starter piece square tables
- Some engines which I got inspiration from (in alphabetical order):
    - [Blunder](https://github.com/algerbrex/blunder)
    - [Leorik](https://github.com/lithander/Leorik)
    - [Stockfish](https://github.com/official-stockfish/Stockfish)
    - [Sunfish](https://github.com/thomasahle/sunfish)
- Rating list testers for testing my engine
