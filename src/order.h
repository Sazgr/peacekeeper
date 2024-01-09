#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

#include "move.h"

int history_bonus(int depth) {
    return depth * depth + depth - 1;
}

struct Move_order_tables {
    Move killer_table[128][2]{};
    int history_successes[12][64]{};
    int history_all[12][64]{};
    int* continuation_successes;
    int* continuation_all;
    Move_order_tables() {
        continuation_successes = new int[12 * 64 * 12 * 64];
        continuation_all = new int[12 * 64 * 12 * 64];
    }
    ~Move_order_tables() {
        delete[] continuation_successes;
        delete[] continuation_all;
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
        for (int i{}; i<12 * 64 * 12 * 64; ++i) {
            continuation_all[i] /= 2;
            continuation_successes[i] /= 2;
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
        if (!history_all[piece][to_square]) return (1 << 11);
        return history_successes[piece][to_square] / history_all[piece][to_square]; //ranges from 0 to 4095
    }
    void continuation_edit(Move previous, Move current, int change, bool success) {
        if (previous.is_null()) return;
        continuation_all[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] += change;
        if (success) continuation_successes[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] += change << 10;
        if (continuation_all[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] > 0x3FFFF) {
            continuation_all[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] /= 2;
            continuation_successes[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] /= 2;
        }
    }
    int continuation_value(Move previous, Move current) {
        if (previous.is_null()) return 512;
        return (8192 + continuation_successes[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()]) / (16 + continuation_all[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()]); //ranges from 0 to 1023
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
