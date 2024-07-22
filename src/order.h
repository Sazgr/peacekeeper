#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

#include "move.h"
#include <algorithm>

int history_bonus(int depth) {
    return 4 * depth * depth;
}

struct Move_order_tables {
    Move killer_table[128][2]{};
    Move counter_table[12][64]{};
    int caphist[13][64]{};
    constexpr static int caphist_max = 1 << 11;
    int history[12][64]{};
    constexpr static int history_max = 1 << 11;
    int butterfly[12][64][64]{};
    constexpr static int butterfly_max = 1 << 11;
    int* continuation;
    constexpr static int continuation_max = 1 << 9;
    Move_order_tables() {
        continuation = new int[12 * 64 * 12 * 64];
    }
    ~Move_order_tables() {
        delete[] continuation;
    }
    void reset() {
        for (int i{}; i<13; ++i) {
            for (int j{}; j<64; ++j) {
                caphist[i][j] = 0;
            }
        }
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j] = 0;
                for (int k{}; k<64; ++k) {
                    butterfly[i][j][k] = 0;
                }
            }
        }
    }
    void age() {
        for (int i{}; i<13; ++i) {
            for (int j{}; j<64; ++j) {
                caphist[i][j] /= 2;
            }
        }
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j] /= 2;
                for (int k{}; k<64; ++k) {
                    butterfly[i][j][k] /= 2;
                }
            }
        }
        for (int i{}; i<12 * 64 * 12 * 64; ++i) {
            continuation[i] /= 2;
        }
    }
    void caphist_edit(Move move, int change, bool success) {
        if (move.captured() == 12 && move.flag() != queen_pr) return;
        change = std::clamp(success ? change : -change, -caphist_max, caphist_max);
        caphist[move.captured()][move.end()] += change - caphist[move.captured()][move.end()] * std::abs(change) / caphist_max;
    }
    int caphist_value(Move move) {
        if (move.captured() == 12 && move.flag() != queen_pr) return 0;
        return caphist[move.captured()][move.end()] + caphist_max; //ranges from 0 to 4095
    }
    void history_edit(Move move, int change, bool success) {
        change = std::clamp(success ? change : -change, -history_max, history_max);
        history[move.piece()][move.end()] += change - history[move.piece()][move.end()] * std::abs(change) / history_max;
    }
    int history_value(Move move) {
        return history[move.piece()][move.end()] + history_max; //ranges from 0 to 4095
    }
    void butterfly_edit(Move move, int change, bool success) {
        change = std::clamp(success ? change : -change, -butterfly_max, butterfly_max);
        butterfly[move.piece()][move.start()][move.end()] += change - butterfly[move.piece()][move.start()][move.end()] * std::abs(change) / butterfly_max;
    }
    int butterfly_value(Move move) {
        return butterfly[move.piece()][move.start()][move.end()] + butterfly_max; //ranges from 0 to 4095
    }
    void continuation_edit(Move previous, Move current, int change, bool success) {
        if (previous.is_null()) return;
        change = std::clamp(success ? change : -change, -continuation_max, continuation_max);
        continuation[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] += change - continuation[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] * std::abs(change) / continuation_max;
    }
    int continuation_value(Move previous, Move current) {
        if (previous.is_null()) return 512;
        return continuation[previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end()] + continuation_max; //ranges from 0 to 1023
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
    void counter_add(Move previous, Move current) {
        counter_table[previous.piece()][previous.end()] = current;
    }
    Move counter_move(Move previous) {
        return counter_table[previous.piece()][previous.end()];
    }
};

#endif
