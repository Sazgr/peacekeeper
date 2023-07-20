#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

#include "move.h"

struct Move_order_tables {
    Move killer_table[128][2]{};
    int history[12][64]{};
    int* continuation;
    Move_order_tables() {
        continuation = new int[12 * 64 * 12 * 64];
    }
    void reset() {
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j] = 0;
            }
        }
    }
    void age() {
        /*for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j] /= 2;
            }
        }
        for (int i{}; i<12 * 64 * 12 * 64; ++i) {
            continuation[i] /= 2;
        }*/
    }
    void history_edit(int piece, int to_square, int change, bool success) {
        if (!success) change = -change;
	history[piece][to_square] -= (history[piece][to_square] * std::abs(change)) / 512;
	history[piece][to_square] += change * 16;
    }
    int history_value(int piece, int to_square) {
        return std::clamp(history[piece][to_square], -65536, 65536);
    }
    void continuation_edit(Move previous, Move current, int change, bool success) {
        if (previous.is_null()) return;
        const int index = previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end();
        if (!success) change = -change;
	continuation[index] -= (continuation[index] * std::abs(change)) / 512;
	continuation[index] += change * 16;
    }
    int continuation_value(Move previous, Move current) {
        if (previous.is_null()) return 0;
        return std::clamp(continuation[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()], -65536, 65536);
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
