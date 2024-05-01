#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

#include "move.h"

int history_bonus(int depth) {
    return depth * depth;
}

struct Move_order_tables {
    Move killer_table[128][2]{};
    Move counter_table[12][64]{};
    int caphist[13][64][2]{};
    int history[12][64][2]{};
    int butterfly[12][64][64][2]{};
    int* continuation;
    Move_order_tables() {
        continuation = new int[12 * 64 * 12 * 64 * 2];
    }
    ~Move_order_tables() {
        delete[] continuation;
    }
    void reset() {
        for (int i{}; i<13; ++i) {
            for (int j{}; j<64; ++j) {
                caphist[i][j][0] = 0;
                caphist[i][j][1] = 0;
            }
        }
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j][0] = 0;
                history[i][j][1] = 0;
                for (int k{}; k<64; ++k) {
                    butterfly[i][j][k][0] = 0;
                    butterfly[i][j][k][1] = 0;
                }
            }
        }
    }
    void age() {
        for (int i{}; i<13; ++i) {
            for (int j{}; j<64; ++j) {
                caphist[i][j][0] /= 2;
                caphist[i][j][1] /= 2;
            }
        }
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                history[i][j][0] /= 2;
                history[i][j][1] /= 2;
                for (int k{}; k<64; ++k) {
                    butterfly[i][j][k][0] /= 2;
                    butterfly[i][j][k][1] /= 2;
                }
            }
        }
        for (int i{}; i<12 * 64 * 12 * 64 * 2; ++i) {
            continuation[i] /= 2;
        }
    }
    void caphist_edit(Move move, int change, bool success) {
        if (move.captured() == 12 && move.flag() != queen_pr) return;
        caphist[move.captured()][move.end()][1] += change;
        if (success) caphist[move.captured()][move.end()][0] += change << 12;
        if (caphist[move.captured()][move.end()][1] > 0x3FFFF) {
            caphist[move.captured()][move.end()][0] /= 2;
            caphist[move.captured()][move.end()][1] /= 2;
        }
    }
    int caphist_value(Move move) {
        if (move.captured() == 12 && move.flag() != queen_pr) return 0;
        if (!caphist[move.captured()][move.end()][1]) return (1 << 11);
        return caphist[move.captured()][move.end()][0] / caphist[move.captured()][move.end()][1]; //ranges from 0 to 4095
    }
    void history_edit(Move move, int change, bool success) {
        history[move.piece()][move.end()][1] += change;
        if (success) history[move.piece()][move.end()][0] += change << 12;
        if (history[move.piece()][move.end()][1] > 0x3FFFF) {
            history[move.piece()][move.end()][0] /= 2;
            history[move.piece()][move.end()][1] /= 2;
        }
    }
    int history_value(Move move) {
        if (!history[move.piece()][move.end()][1]) return (1 << 11);
        return history[move.piece()][move.end()][0] / history[move.piece()][move.end()][1]; //ranges from 0 to 4095
    }
    void butterfly_edit(Move move, int change, bool success) {
        butterfly[move.piece()][move.start()][move.end()][1] += change;
        if (success) butterfly[move.piece()][move.start()][move.end()][0] += change << 12;
        if (butterfly[move.piece()][move.start()][move.end()][1] > 0x3FFFF) {
            butterfly[move.piece()][move.start()][move.end()][0] /= 2;
            butterfly[move.piece()][move.start()][move.end()][1] /= 2;
        }
    }
    int butterfly_value(Move move) {
        if (!butterfly[move.piece()][move.start()][move.end()][1]) return (1 << 11);
        return butterfly[move.piece()][move.start()][move.end()][0] / butterfly[move.piece()][move.start()][move.end()][1]; //ranges from 0 to 4095
    }
    void continuation_edit(Move previous, Move current, int change, bool success) {
        if (previous.is_null()) return;
        int index = previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end();
        continuation[2 * index + 1] += change;
        if (success) continuation[2 * index] += change << 10;
        if (continuation[2 * index + 1] > 0x3FFFF) {
            continuation[2 * index] /= 2;
            continuation[2 * index + 1] /= 2;
        }
    }
    int continuation_value(Move previous, Move current) {
        if (previous.is_null()) return 512;
        int index = previous.piece() * 64 * 12 * 64 + previous.end() * 12 * 64 + current.piece() * 64 + current.end();
        return (8192 + continuation[2 * index]) / (16 + continuation[2 * index + 1]); //ranges from 0 to 1023
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
