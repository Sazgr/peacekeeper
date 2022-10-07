## Peacekeeper
UCI Chess Engine in Progress

### Features:
- Move Generation
    - Bitboard-based
    - Fixed-shift Fancy Magic Bitboards for slider move generation
- Search
    - Principal Variation Search (PVS)
    - Quiescence Search
    - Iterative Deepening
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
- Evaluation
    - Piece Square Tables (PST)
        - Incrementally updated
    - Tapered Eval
    - Tempo Bonus
