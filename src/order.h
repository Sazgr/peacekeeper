#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

#include "move.h"

struct Move_order_tables {
    Move killer_table[128][2]{};
    int history[12][64]{};
    int* continuation;
    Move_order_tables() {
        continuation = new int[12 * 64 * 12 * 64];
        reset();
    }
    void reset() {
        for (int i{}; i < 12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j] = -4096;
            }
        }
        for (int i{}; i < 12 * 64 * 12 * 64; ++i) {
            continuation[i] = -1024;
        }
    }
    void age() {
        for (int i{}; i < 12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j] /= 2;
            }
        }
        for (int i{}; i < 12 * 64 * 12 * 64; ++i) {
            continuation[i] /= 2;
        }
    }
    void history_edit(int piece, int to_square, int change, bool success) {
	history[piece][to_square] -= (history[piece][to_square] * change) / 256;
        if (!success) change = -change;
	history[piece][to_square] += change * 32;
    }
    int history_value(int piece, int to_square) {
        return history[piece][to_square];
    }
    void continuation_edit(Move previous, Move current, int change, bool success) {
        if (previous.is_null()) return;
        const int index = previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end();
	continuation[index] -= (continuation[index] * change) / 256;
        if (!success) change = -change;
	continuation[index] += change * 8;
    }
    int continuation_value(Move previous, Move current) {
        if (previous.is_null()) return -1024;
        return continuation[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()];
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