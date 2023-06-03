#include "eval.h"

int middlegame[13][64]{};
int endgame[13][64]{};

void pst_init() {
    for (int pc{0}; pc<6; ++pc) {
        for (int sq{0}; sq<64; ++sq) {
            middlegame[pc * 2][sq ^ 56] = -mg_value[pc] - mg_table[pc][sq];
            middlegame[pc * 2 + 1][sq] = mg_value[pc] + mg_table[pc][sq];
            endgame   [pc * 2][sq ^ 56] = -eg_value[pc] - eg_table[pc][sq];
            endgame   [pc * 2 + 1][sq] = eg_value[pc] + eg_table[pc][sq];
        }
    }
    for (int sq{0}; sq<64; ++sq) {
        middlegame[12][sq] = 0;
        endgame[12][sq] = 0;
    }
}
