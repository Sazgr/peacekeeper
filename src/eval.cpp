#include "eval.h"

int middlegame[2][64][10][64]{};
int endgame[2][64][10][64]{};

void pst_init() {
    for (int ksq{0}; ksq<32; ++ksq) {
        int eksq = ((ksq & ~3) << 1) + (ksq & 3);
        for (int opp{0}; opp<2; ++opp) {
            for (int pc{0}; pc<5; ++pc) {
                for (int sq{0}; sq<64; ++sq) {
                    middlegame[opp][eksq][pc*2][sq] = - mg_table[opp][ksq ^ 28][pc][sq ^ 56];
                    endgame[opp][eksq][pc*2][sq] = - eg_table[opp][ksq ^ 28][pc][sq ^ 56];
                    middlegame[opp][eksq ^ 7][pc*2][sq ^ 7] = - mg_table[opp][ksq ^ 28][pc][sq ^ 56];
                    endgame[opp][eksq ^ 7][pc*2][sq ^ 7] = - eg_table[opp][ksq ^ 28][pc][sq ^ 56];
                }
                for (int sq{0}; sq<64; ++sq) {
                    middlegame[opp ^ 1][eksq][pc*2+1][sq] = mg_table[opp][ksq][pc][sq];
                    endgame[opp ^ 1][eksq][pc*2+1][sq] = eg_table[opp][ksq][pc][sq];
                    middlegame[opp ^ 1][eksq ^ 7][pc*2+1][sq ^ 7] = mg_table[opp][ksq][pc][sq];
                    endgame[opp ^ 1][eksq ^ 7][pc*2+1][sq ^ 7] = eg_table[opp][ksq][pc][sq];
                }
            }
        }
    }
}
