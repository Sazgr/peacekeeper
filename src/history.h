#ifndef PEACEKEEPER_HISTORY
#define PEACEKEEPER_HISTORY

struct History_table {
    int table[12][64]{};
    int sum{};
    void edit(int piece, int to_square, int change) {
        table[piece][to_square] += change;
        sum += change;
        if (sum >= 0xFFFFFF || sum <= -0xFFFFFF) age();
    }
    void age() {
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                table[i][j] /= 2;
            }
        }
        sum /= 2;
    }
    void reset() {
        for (int i{}; i<12; ++i) {
            for (int j{}; j<64; ++j) {
                table[i][j] = 0;
            }
        }
        sum = 0;
    }
};

#endif
