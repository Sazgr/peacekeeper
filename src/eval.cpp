#include "eval.h"

int full_king[2][64][10][64]{};

void pst_init() {
    for (int ksq{0}; ksq<32; ++ksq) {
        int eksq = ((ksq & ~3) << 1) + (ksq & 3);
        for (int opp{0}; opp<2; ++opp) {
            for (int pc{0}; pc<5; ++pc) {
                for (int sq{0}; sq<64; ++sq) {
                    full_king[opp][eksq][pc*2][sq] = - table[opp][ksq ^ 28][pc][sq ^ 56];
                    full_king[opp][eksq ^ 7][pc*2][sq ^ 7] = - table[opp][ksq ^ 28][pc][sq ^ 56];
                }
                for (int sq{0}; sq<64; ++sq) {
                    full_king[opp ^ 1][eksq][pc*2+1][sq] = table[opp][ksq][pc][sq];
                    full_king[opp ^ 1][eksq ^ 7][pc*2+1][sq ^ 7] = table[opp][ksq][pc][sq];
                }
            }
        }
    }
}
