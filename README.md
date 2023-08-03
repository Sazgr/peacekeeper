![Logo](peacekeeper-large.jpg)

## Peacekeeper

A fairly strong UCI Chess Engine written in C++17.

### Engine Issues

Issues on github can be created for any bugs or other issues found.

### Compilation

Peacekeeper can be compiled from source with GNU C++ or Clang with `g++ -Ofast -DNDEBUG -march=native -o peacekeeper src/*.cpp`
MSVC intrinsics are supported as well as C++20 bit intrinsics, so the engine can be compiled on a large range of compilers.

Windows (and formerly linux) executables are provided in the [Releases](https://github.com/Sazgr/peacekeeper/releases) section.

### Ratings

Version | CCRL Blitz | CCRL 40/15
--------|------------|-----------
v1.10   | 2448       | 2381
v1.20   | 2563       | 2474
v1.30   | 2646       | 2608
v1.40   | 2733       | 2694
v1.50   | 2819       | 2768
v1.60   | 3057       | ~2971

### Features

This is a list of features I have implemented so far in Peacekeeper. It may be incomplete or outdated at times.

- Board Representation and Move Generation
    - Bitboard-based
    - Redundant Mailbox Board
    - Make-Unmake
    - Fixed-shift Fancy Magic Bitboards for slider move generation
    - Fully legal move generation
- Search
    - Negamax framework
    - Principal Variation Search (PVS)
    - Quiescence Search (QS)
        - QS SEE Pruning
    - Iterative Deepening
        - Aspiration Windows
    - Staged Move Generation
        - Hash move
        - Captures
        - Quiets
    - Time Management
        - Soft and Hard Bounds
        - More time for unstable bestmoves
    - Transposition Table
        - Incrementally updated Zobrist hash
        - Used in both QS and PVS
    - Selectivity
        - Check Extensions
        - Static Null Move Pruning/Reverse Futility Pruning (SNMP/RFP)
        - Null Move Pruning (NMP)
        - Late Move Reductions (LMR)
        - Futility Pruning
        - Delta Pruning
        - SEE Pruning
        - Internal Iterative Reductions (IIR)
    - Move Ordering
        - MVV-LVA for captures
        - Killer Heuristic
        - History Heuristic for quiet moves
        - Continuation and Countermoves History
- Evaluation
    - Texel Tuned using Gradient Descent tuner
    - King-Relative Piece Square Tables
    - Pawn structure
        - Passed Pawns
        - Free Passed Pawns
        - Doubled Pawns
        - Isolated Pawns
        - Supported Pawns
        - Pawn Phalanxes
    - Mobility
        - Regular mobility
        - Forward mobility
    - Bishop Pair
    - Open and Semi-Open Files
    - Tapered Eval
    - Tempo Bonus
        - Phase dependent
    - Contempt
        - Phase dependent

### Credits & Thanks
In no particular order.

- Pradu Kannan for magic multipliers
- CPW (and Sungorus) for zobrist hash pseudorandom number generator
- The [Chess Programming Wiki](https://www.chessprogramming.org) for being a great resource for everything related to chess programming
- The [Talkchess](https://talkchess.com) forum and the people on it for answering my more specific questions
- Engine Programming Discord and the people on it
- Slender (@rafid-dev) especially for coinhabiting an OB, contributing hardware mutually, and sharing SSS test results.
- Andrew Zhuo (@StackFish5) for constantly looking over my code
- PeSTO for starter piece square tables
- [Chess cache](https://www.chesscache.com/ChessEngines.html) (Dusan Stamenkovic) and Graham from CCRL for logos
- Some engines which I got inspiration from (in alphabetical order) along with their authors:
    - [Altair](https://github.com/Alex2262/AltairChessEngine)
    - [Blunder](https://github.com/algerbrex/blunder)
    - [Leorik](https://github.com/lithander/Leorik)
    - [Midnight](https://github.com/archishou/MidnightChessEngine)
    - [Polaris](https://github.com/Ciekce/Polaris)
    - [Rice](https://github.com/rafid-dev/rice)
    - [Stockfish](https://github.com/official-stockfish/Stockfish)
    - [Sunfish](https://github.com/thomasahle/sunfish)
    - [Willow](https://github.com/Adam-Kulju/Willow)
- Rating list testers for testing my engine
