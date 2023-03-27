#include "eval.h"

int middlegame[64][2][10][64]{};
int endgame[64][2][10][64]{};

void pst_init() {
    for (int ksq{0}; ksq<32; ++ksq) {
        int eksq = ((ksq & ~3) << 1) + (ksq & 3);
        for (int opp{0}; opp<2; ++opp) {
            for (int pc{0}; pc<5; ++pc) {
                for (int sq{0}; sq<64; ++sq) {
                    middlegame[eksq][opp][pc*2][sq] = - mg_table[ksq ^ 28][opp][pc][sq ^ 56];
                    endgame[eksq][opp][pc*2][sq] = - eg_table[ksq ^ 28][opp][pc][sq ^ 56];
                    middlegame[eksq ^ 7][opp][pc*2][sq ^ 7] = - mg_table[ksq ^ 28][opp][pc][sq ^ 56];
                    endgame[eksq ^ 7][opp][pc*2][sq ^ 7] = - eg_table[ksq ^ 28][opp][pc][sq ^ 56];
                }
                for (int sq{0}; sq<64; ++sq) {
                    middlegame[eksq][opp ^ 1][pc*2+1][sq] = mg_table[ksq][opp][pc][sq];
                    endgame[eksq][opp ^ 1][pc*2+1][sq] = eg_table[ksq][opp][pc][sq];
                    middlegame[eksq ^ 7][opp ^ 1][pc*2+1][sq ^ 7] = mg_table[ksq][opp][pc][sq];
                    endgame[eksq ^ 7][opp ^ 1][pc*2+1][sq ^ 7] = eg_table[ksq][opp][pc][sq];
                }
            }
        }
    }
}
