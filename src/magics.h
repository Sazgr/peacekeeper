#ifndef PEACEKEEPER_MAGICS
#define PEACEKEEPER_MAGICS

#include "typedefs.h"

const u64 rook_premask[64] {
    0x101010101017e,    0x202020202027c,    0x404040404047a,    0x8080808080876,    0x1010101010106e,   0x2020202020205e,   0x4040404040403e,   0x8080808080807e,
    0x1010101017e00,    0x2020202027c00,    0x4040404047a00,    0x8080808087600,    0x10101010106e00,   0x20202020205e00,   0x40404040403e00,   0x80808080807e00,
    0x10101017e0100,    0x20202027c0200,    0x40404047a0400,    0x8080808760800,    0x101010106e1000,   0x202020205e2000,   0x404040403e4000,   0x808080807e8000,
    0x101017e010100,    0x202027c020200,    0x404047a040400,    0x8080876080800,    0x1010106e101000,   0x2020205e202000,   0x4040403e404000,   0x8080807e808000,
    0x1017e01010100,    0x2027c02020200,    0x4047a04040400,    0x8087608080800,    0x10106e10101000,   0x20205e20202000,   0x40403e40404000,   0x80807e80808000,
    0x17e0101010100,    0x27c0202020200,    0x47a0404040400,    0x8760808080800,    0x106e1010101000,   0x205e2020202000,   0x403e4040404000,   0x807e8080808000,
    0x7e010101010100,   0x7c020202020200,   0x7a040404040400,   0x76080808080800,   0x6e101010101000,   0x5e202020202000,   0x3e404040404000,   0x7e808080808000,
    0x7e01010101010100, 0x7c02020202020200, 0x7a04040404040400, 0x7608080808080800, 0x6e10101010101000, 0x5e20202020202000, 0x3e40404040404000, 0x7e80808080808000,
};
const u64 bishop_premask[64] {
    0x40201008040200,   0x402010080400,     0x4020100a00,       0x40221400,         0x2442800,          0x204085000,        0x20408102000,      0x2040810204000,
    0x20100804020000,   0x40201008040000,   0x4020100a0000,     0x4022140000,       0x244280000,        0x20408500000,      0x2040810200000,    0x4081020400000,
    0x10080402000200,   0x20100804000400,   0x4020100a000a00,   0x402214001400,     0x24428002800,      0x2040850005000,    0x4081020002000,    0x8102040004000,
    0x8040200020400,    0x10080400040800,   0x20100a000a1000,   0x40221400142200,   0x2442800284400,    0x4085000500800,    0x8102000201000,    0x10204000402000,
    0x4020002040800,    0x8040004081000,    0x100a000a102000,   0x22140014224000,   0x44280028440200,   0x8500050080400,    0x10200020100800,   0x20400040201000,
    0x2000204081000,    0x4000408102000,    0xa000a10204000,    0x14001422400000,   0x28002844020000,   0x50005008040200,   0x20002010080400,   0x40004020100800,
    0x20408102000,      0x40810204000,      0xa1020400000,      0x142240000000,     0x284402000000,     0x500804020000,     0x201008040200,     0x402010080400,
    0x2040810204000,    0x4081020400000,    0xa102040000000,    0x14224000000000,   0x28440200000000,   0x50080402000000,   0x20100804020000,   0x40201008040200,
};

const struct {u64 magic; int start;} bishop_magics[64] {
    { 0x007bfeffbfeffbffull,  16530 },
    { 0x003effbfeffbfe08ull,   9162 },
    { 0x0000401020200000ull,   9674 },
    { 0x0000200810000000ull,  18532 },
    { 0x0000110080000000ull,  19172 },
    { 0x0000080100800000ull,  17700 },
    { 0x0007efe0bfff8000ull,   5730 },
    { 0x00000fb0203fff80ull,  19661 },
    { 0x00007dff7fdff7fdull,  17065 },
    { 0x0000011fdff7efffull,  12921 },
    { 0x0000004010202000ull,  15683 },
    { 0x0000002008100000ull,  17764 },
    { 0x0000001100800000ull,  19684 },
    { 0x0000000801008000ull,  18724 },
    { 0x000007efe0bfff80ull,   4108 },
    { 0x000000080f9fffc0ull,  12936 },
    { 0x0000400080808080ull,  15747 },
    { 0x0000200040404040ull,   4066 },
    { 0x0000400080808080ull,  14359 },
    { 0x0000200200801000ull,  36039 },
    { 0x0000240080840000ull,  20457 },
    { 0x0000080080840080ull,  43291 },
    { 0x0000040010410040ull,   5606 },
    { 0x0000020008208020ull,   9497 },
    { 0x0000804000810100ull,  15715 },
    { 0x0000402000408080ull,  13388 },
    { 0x0000804000810100ull,   5986 },
    { 0x0000404004010200ull,  11814 },
    { 0x0000404004010040ull,  92656 },
    { 0x0000101000804400ull,   9529 },
    { 0x0000080800104100ull,  18118 },
    { 0x0000040400082080ull,   5826 },
    { 0x0000410040008200ull,   4620 },
    { 0x0000208020004100ull,  12958 },
    { 0x0000110080040008ull,  55229 },
    { 0x0000020080080080ull,   9892 },
    { 0x0000404040040100ull,  33767 },
    { 0x0000202040008040ull,  20023 },
    { 0x0000101010002080ull,   6515 },
    { 0x0000080808001040ull,   6483 },
    { 0x0000208200400080ull,  19622 },
    { 0x0000104100200040ull,   6274 },
    { 0x0000208200400080ull,  18404 },
    { 0x0000008840200040ull,  14226 },
    { 0x0000020040100100ull,  17990 },
    { 0x007fff80c0280050ull,  18920 },
    { 0x0000202020200040ull,  13862 },
    { 0x0000101010100020ull,  19590 },
    { 0x0007ffdfc17f8000ull,   5884 },
    { 0x0003ffefe0bfc000ull,  12946 },
    { 0x0000000820806000ull,   5570 },
    { 0x00000003ff004000ull,  18740 },
    { 0x0000000100202000ull,   6242 },
    { 0x0000004040802000ull,  12326 },
    { 0x007ffeffbfeff820ull,   4156 },
    { 0x003fff7fdff7fc10ull,  12876 },
    { 0x0003ffdfdfc27f80ull,  17047 },
    { 0x000003ffefe0bfc0ull,  17780 },
    { 0x0000000008208060ull,   2494 },
    { 0x0000000003ff0040ull,  17716 },
    { 0x0000000001002020ull,  17067 },
    { 0x0000000040408020ull,   9465 },
    { 0x00007ffeffbfeff9ull,  16196 },
    { 0x007ffdff7fdff7fdull,   6166 }
};

const struct {u64 magic; int start;} rook_magics[64] {
    { 0x00a801f7fbfeffffull,  85487 },
    { 0x00180012000bffffull,  43101 },
    { 0x0040080010004004ull,      0 },
    { 0x0040040008004002ull,  49085 },
    { 0x0040020004004001ull,  93168 },
    { 0x0020008020010202ull,  78956 },
    { 0x0040004000800100ull,  60703 },
    { 0x0810020990202010ull,  64799 },
    { 0x000028020a13fffeull,  30640 },
    { 0x003fec008104ffffull,   9256 },
    { 0x00001800043fffe8ull,  28647 },
    { 0x00001800217fffe8ull,  10404 },
    { 0x0000200100020020ull,  63775 },
    { 0x0000200080010020ull,  14500 },
    { 0x0000300043ffff40ull,  52819 },
    { 0x000038010843fffdull,   2048 },
    { 0x00d00018010bfff8ull,  52037 },
    { 0x0009000c000efffcull,  16435 },
    { 0x0004000801020008ull,  29104 },
    { 0x0002002004002002ull,  83439 },
    { 0x0001002002002001ull,  86842 },
    { 0x0001001000801040ull,  27623 },
    { 0x0000004040008001ull,  26599 },
    { 0x0000802000200040ull,  89583 },
    { 0x0040200010080010ull,   7042 },
    { 0x0000080010040010ull,  84463 },
    { 0x0004010008020008ull,  82415 },
    { 0x0000020020040020ull,  95216 },
    { 0x0000010020020020ull,  35015 },
    { 0x0000008020010020ull,  10790 },
    { 0x0000008020200040ull,  53279 },
    { 0x0000200020004081ull,  70684 },
    { 0x0040001000200020ull,  38640 },
    { 0x0000080400100010ull,  32743 },
    { 0x0004010200080008ull,  68894 },
    { 0x0000200200200400ull,  62751 },
    { 0x0000200100200200ull,  41670 },
    { 0x0000200080200100ull,  25575 },
    { 0x0000008000404001ull,   3042 },
    { 0x0000802000200040ull,  36591 },
    { 0x00ffffb50c001800ull,  69918 },
    { 0x007fff98ff7fec00ull,   9092 },
    { 0x003ffff919400800ull,  17401 },
    { 0x001ffff01fc03000ull,  40688 },
    { 0x0000010002002020ull,  96240 },
    { 0x0000008001002020ull,  91632 },
    { 0x0003fff673ffa802ull,  32495 },
    { 0x0001fffe6fff9001ull,  51133 },
    { 0x00ffffd800140028ull,  78319 },
    { 0x007fffe87ff7ffecull,  12595 },
    { 0x003fffd800408028ull,   5152 },
    { 0x001ffff111018010ull,  32110 },
    { 0x000ffff810280028ull,  13894 },
    { 0x0007fffeb7ff7fd8ull,   2546 },
    { 0x0003fffc0c480048ull,  41052 },
    { 0x0001ffffa2280028ull,  77676 },
    { 0x00ffffe4ffdfa3baull,  73580 },
    { 0x007ffb7fbfdfeff6ull,  44947 },
    { 0x003fffbfdfeff7faull,  73565 },
    { 0x001fffeff7fbfc22ull,  17682 },
    { 0x000ffffbf7fc2ffeull,  56607 },
    { 0x0007fffdfa03ffffull,  56135 },
    { 0x0003ffdeff7fbdecull,  44989 },
    { 0x0001ffff99ffab2full,  21479 }
};

extern u64 lookup_table[97264];

#endif