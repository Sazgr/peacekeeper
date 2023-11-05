#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

#include "move.h"

int history_bonus(int depth) {
    return depth * depth;
}
struct Move_order_tables {
    Move killer_table[128][2]{};
    int history_successes[12][64]{};
    int history_all[12][64]{};
    Move_order_tables() {
    }
    ~Move_order_tables() {
    }
    void reset() {
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history_all[i][j] = 0;
                history_successes[i][j] = 0;
            }
        }
    }
    void age() {
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history_all[i][j] /= 2;
                history_successes[i][j] /= 2;
            }
        }
    }
    void history_edit(int piece, int to_square, int change, bool success) {
        history_all[piece][to_square] += change;
        if (success) history_successes[piece][to_square] += change << 12;
        if (history_all[piece][to_square] > 0x3FFFF) {
            history_all[piece][to_square] /= 2;
            history_successes[piece][to_square] /= 2;
        }
    }
    int history_value(int piece, int to_square) {
        if (!history_all[piece][to_square]) return (1 << 10);
        return history_successes[piece][to_square] / history_all[piece][to_square]; //ranges from 0 to 4095
    }
    void continuation_edit(Move previous, Move current, int change, bool success) {
        return;
    }
    int continuation_value(Move previous, Move current) {
        return 256;
    }
    void killer_add(Move move, int ply) {
        if (killer_table[ply][0] != move) {
            killer_table[ply][1] = killer_table[ply][0];
            killer_table[ply][0] = move;
        }
    }
    Move killer_move(int ply, int index) {
        return killer_table[ply][index];
    }
};

#endif
