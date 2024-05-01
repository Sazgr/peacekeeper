#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

#include "move.h"
#include <algorithm>

int history_bonus(int depth) {
    return std::min(depth * depth, 100);
}

struct Move_order_tables {
    Move killer_table[128][2]{};
    Move counter_table[12][64]{};
    int caphist_successes[13][64]{};
    int caphist_all[13][64]{};
    int history_successes[12][64]{};
    int history_all[12][64]{};
    int butterfly_successes[12][64][64]{};
    int butterfly_all[12][64][64]{};
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
        for (int i{}; i<13; ++i) {
            for (int j{}; j<64; ++j) {
                caphist_all[i][j] = 0;
                caphist_successes[i][j] = 0;
            }
        }
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history_all[i][j] = 0;
                history_successes[i][j] = 0;
                for (int k{}; k<64; ++k) {
                    butterfly_all[i][j][k] = 0;
                    butterfly_successes[i][j][k] = 0;
                }
            }
        }
    }
    void age() {
        for (int i{}; i<13; ++i) {
            for (int j{}; j<64; ++j) {
                caphist_all[i][j] /= 2;
                caphist_successes[i][j] /= 2;
            }
        }
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history_all[i][j] /= 2;
                history_successes[i][j] /= 2;
                for (int k{}; k<64; ++k) {
                    butterfly_all[i][j][k] /= 2;
                    butterfly_successes[i][j][k] /= 2;
                }
            }
        }
        for (int i{}; i<12 * 64 * 12 * 64; ++i) {
            continuation_all[i] /= 2;
            continuation_successes[i] /= 2;
        }
    }
    void caphist_edit(Move move, int change, bool success) {
        if (move.captured() == 12 && move.flag() != queen_pr) return;
        caphist_all[move.captured()][move.end()] += change;
        if (success) caphist_successes[move.captured()][move.end()] += change << 12;
        if (caphist_all[move.captured()][move.end()] > 0x3FFFF) {
            caphist_all[move.captured()][move.end()] /= 2;
            caphist_successes[move.captured()][move.end()] /= 2;
        }
    }
    int caphist_value(Move move) {
        if (move.captured() == 12 && move.flag() != queen_pr) return 0;
        if (!caphist_all[move.captured()][move.end()]) return (1 << 11);
        return caphist_successes[move.captured()][move.end()] / caphist_all[move.captured()][move.end()]; //ranges from 0 to 4095
    }
    void history_edit(Move move, int change, bool success) {
        history_all[move.piece()][move.end()] += change;
        if (success) history_successes[move.piece()][move.end()] += change << 12;
        if (history_all[move.piece()][move.end()] > 0x3FFFF) {
            history_all[move.piece()][move.end()] /= 2;
            history_successes[move.piece()][move.end()] /= 2;
        }
    }
    int history_value(Move move) {
        if (!history_all[move.piece()][move.end()]) return (1 << 11);
        return history_successes[move.piece()][move.end()] / history_all[move.piece()][move.end()]; //ranges from 0 to 4095
    }
    void butterfly_edit(Move move, int change, bool success) {
        butterfly_all[move.piece()][move.start()][move.end()] += change;
        if (success) butterfly_successes[move.piece()][move.start()][move.end()] += change << 12;
        if (butterfly_all[move.piece()][move.start()][move.end()] > 0x3FFFF) {
            butterfly_all[move.piece()][move.start()][move.end()] /= 2;
            butterfly_successes[move.piece()][move.start()][move.end()] /= 2;
        }
    }
    int butterfly_value(Move move) {
        if (!butterfly_all[move.piece()][move.start()][move.end()]) return (1 << 11);
        return butterfly_successes[move.piece()][move.start()][move.end()] / butterfly_all[move.piece()][move.start()][move.end()]; //ranges from 0 to 4095
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
    void counter_add(Move previous, Move current) {
        counter_table[previous.piece()][previous.end()] = current;
    }
    Move counter_move(Move previous) {
        return counter_table[previous.piece()][previous.end()];
    }
};

#endif
