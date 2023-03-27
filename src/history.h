#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

struct History_table {
    int successes[12][64]{};
    int all[12][64]{};
    void edit(int piece, int to_square, int change, bool success) {
        all[piece][to_square] += change;
        if (success) successes[piece][to_square] += change << 12;
        if (all[piece][to_square] > 0x3FFFF) {
            all[piece][to_square] /= 2;
            successes[piece][to_square] /= 2;
        }
    }
    int value(int piece, int to_square) {
        if (!all[piece][to_square]) return (1 << 10);
        return successes[piece][to_square] / all[piece][to_square]; //ranges from 0 to 4095
    }
    void age() {
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                all[i][j] /= 2;
                successes[i][j] /= 2;
            }
        }
    }
    void reset() {
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                all[i][j] = 0;
                successes[i][j] = 0;
            }
        }
    }
};

#endif
