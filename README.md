## Peacekeeper
UCI Chess Engine in Progress

### Features:
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
