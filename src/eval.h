#ifndef PEACEKEEPER_EVAL
#define PEACEKEEPER_EVAL

const int flag_priority[7] = {  0, 254, 282, 394, 939, 38, 85};

const int mg_value[7] = {100, 335, 369, 485, 1051, 20000, 0};
const int eg_value[7] = {123, 270, 300, 529, 993, 20000, 0};

const int mg_table[6][64] = {{
    0, 0, 0, 0, 0, 0, 0, 0,
    63, 93, 66, 89, 62, 56, -22, -45,
    -20, -8, 23, 23, 35, 63, 42, 4,
    -40, -28, -15, -12, 8, 3, -2, -13,
    -44, -35, -20, -4, -4, -6, -14, -22,
    -42, -38, -22, -18, -4, -7, 3, -15,
    -44, -38, -31, -29, -17, 2, 11, -24,
    0, 0, 0, 0, 0, 0, 0, 0,
}, {
    -182, -108, -56, -33, 32, -74, -36, -121,
    -22, -2, 23, 48, 22, 93, -5, 23,
    0, 32, 49, 60, 103, 112, 59, 32,
    -1, 9, 33, 53, 34, 60, 22, 37,
    -13, -2, 15, 18, 27, 22, 19, -2,
    -33, -12, 1, 11, 24, 7, 9, -15,
    -41, -29, -17, -2, -2, 2, -11, -18,
    -96, -32, -33, -18, -16, -9, -30, -58,
}, {
    -31, -42, -56, -80, -60, -60, -15, -52,
    -16, 2, -4, -23, 10, 1, -2, -13,
    -2, 19, 15, 33, 23, 63, 39, 30,
    -9, 2, 18, 30, 25, 19, 5, -7,
    -8, -6, -4, 22, 17, -1, -6, 6,
    -1, 3, 6, 1, 5, 8, 8, 14,
    6, 7, 9, -6, 3, 15, 23, 11,
    -1, 8, -1, -8, 0, -8, 15, 12,
}, {
    20, 6, 8, 7, 26, 35, 36, 65,
    2, -5, 17, 40, 26, 56, 50, 85,
    -15, 7, 1, 3, 37, 49, 100, 73,
    -23, -14, -15, -12, -8, 4, 20, 19,
    -41, -42, -31, -25, -23, -28, -4, -18,
    -44, -38, -31, -28, -20, -20, 9, -14,
    -44, -37, -19, -21, -15, -11, 5, -27,
    -30, -24, -15, -9, -4, -11, 1, -27,
}, {
    -33, -28, 2, 23, 32, 45, 59, 13,
    -5, -35, -23, -29, -22, 15, 0, 52,
    -2, -6, -13, 1, 11, 63, 70, 69,
    -19, -15, -13, -17, -14, 0, 8, 11,
    -15, -19, -19, -12, -12, -11, -2, 5,
    -14, -8, -12, -13, -9, -4, 10, 4,
    -13, -10, -1, 0, -1, 9, 15, 26,
    -10, -20, -13, -5, -8, -15, 1, 1,
}, {
    -37, 37, 36, -7, -35, -9, 29, 26,
    16, 9, -14, 26, 12, 17, 6, -1,
    -23, 45, -2, -20, 1, 51, 49, 2,
    -22, -31, -42, -80, -80, -45, -39, -66,
    -44, -36, -73, -106, -102, -61, -70, -100,
    3, 22, -38, -51, -43, -40, 1, -17,
    95, 53, 37, 3, -1, 19, 63, 73,
    91, 116, 91, -6, 57, 21, 93, 93,
}};

const int eg_table[6][64] = {{
    0, 0, 0, 0, 0, 0, 0, 0,
    142, 122, 128, 74, 76, 90, 136, 151,
    24, 20, -18, -56, -60, -39, -4, 4,
    8, -5, -20, -40, -40, -31, -18, -18,
    -15, -18, -29, -36, -35, -31, -28, -32,
    -18, -20, -27, -25, -25, -27, -32, -35,
    -19, -19, -23, -24, -17, -26, -35, -37,
    0, 0, 0, 0, 0, 0, 0, 0,
}, {
    -71, -25, -2, -9, -16, -24, -36, -95,
    -20, -3, 5, 4, 1, -20, -7, -38,
    -9, 8, 33, 33, 14, 6, -3, -20,
    4, 24, 45, 46, 46, 41, 23, -8,
    2, 17, 45, 46, 49, 36, 15, -6,
    -13, 9, 25, 39, 37, 21, 3, -11,
    -21, -6, 8, 7, 5, 2, -16, -14,
    -25, -35, -12, -12, -8, -18, -28, -33,
}, {
    0, 8, 6, 14, 8, 0, -4, -4,
    -8, -3, 0, 3, -9, -7, -1, -12,
    8, -2, 3, -7, -3, -1, -4, 2,
    4, 8, 3, 16, 10, 8, 5, 4,
    0, 7, 14, 13, 11, 9, 5, -13,
    0, 5, 8, 9, 13, 6, -5, -10,
    -3, -7, -13, 1, -1, -9, -3, -21,
    -13, -3, -4, -3, -5, 5, -18, -26,
}, {
    12, 18, 25, 20, 13, 9, 9, 0,
    13, 24, 26, 14, 15, 0, -3, -14,
    11, 11, 13, 9, -6, -13, -23, -23,
    13, 11, 17, 14, 0, -7, -8, -12,
    9, 11, 10, 8, 5, 2, -9, -9,
    3, 1, 0, 1, -4, -11, -29, -24,
    -5, 0, -3, -2, -10, -15, -24, -14,
    -4, -2, 2, -1, -8, -7, -13, -14,
}, {
    4, 14, 24, 22, 14, 8, -28, -1,
    -15, 21, 46, 63, 78, 35, 23, 7,
    -13, -4, 32, 35, 49, 19, -13, -16,
    1, 9, 18, 41, 52, 40, 33, 22,
    -10, 11, 14, 31, 28, 24, 12, 7,
    -25, -16, 2, -1, 3, 0, -18, -22,
    -29, -29, -37, -27, -25, -52, -76, -90,
    -33, -28, -31, -37, -35, -36, -53, -59,
}, {
    -78, -51, -32, -4, -5, 1, 0, -81,
    -23, 13, 23, 18, 31, 42, 40, 9,
    -7, 18, 34, 44, 47, 44, 38, 11,
    -12, 21, 40, 52, 53, 48, 37, 11,
    -19, 10, 35, 51, 50, 36, 24, 9,
    -30, -7, 17, 29, 28, 19, 0, -12,
    -53, -22, -11, 0, 4, -5, -23, -42,
    -91, -69, -48, -28, -51, -31, -59, -89,
}};

const int mg_knight_mobility[9] = {-1, -1, 2, 0, 3, 6, 8, 10, 5};
const int mg_bishop_mobility[14] = {-41, -19, -11, -9, -2, 6, 9, 15, 15, 18, 19, 25, 46, 61};
const int mg_rook_mobility[15] = {14, -12, -6, -7, -6, -5, -1, 3, 11, 18, 23, 27, 31, 43, 47};
const int mg_queen_mobility[28] = {0, 21, 63, 64, 58, 56, 56, 57, 58, 59, 60, 63, 63, 62, 63, 64, 62, 62, 63, 63, 68, 68, 84, 93, 105, 130, 100, 78};

const int eg_knight_mobility[9] = {12, 59, 65, 66, 65, 67, 60, 54, 52};
const int eg_bishop_mobility[14] = {5, 9, 14, 31, 41, 49, 56, 60, 66, 65, 65, 61, 49, 54};
const int eg_rook_mobility[15] = {26, 58, 55, 64, 75, 81, 84, 86, 85, 88, 90, 94, 98, 96, 97};
const int eg_queen_mobility[28] = {0, 0, -4, -5, 29, 72, 93, 107, 114, 128, 126, 126, 130, 136, 140, 143, 150, 154, 158, 163, 163, 168, 156, 158, 160, 152, 138, 128};

const int mg_passed[8] = {0, -4, 16, 14, -2, -6, 1, 0};
const int mg_passed_free[8] = {0, -10, 0, 11, -8, -8, -1, 0};
const int mg_doubled[8] = {0, 0, -2, 5, -7, -8, -3, 0};
const int mg_isolated[8] = {0, 15, 3, -3, -17, -23, -15, 0};
const int eg_passed[8] = {0, -37, 59, 43, 29, 16, 6, 0};
const int eg_passed_free[8] = {0, 43, 137, 65, 38, 13, 9, 0};
const int eg_doubled[8] = {0, 0, 4, 0, -9, -10, -16, 0};
const int eg_isolated[8] = {0, -7, -23, -20, -10, -12, -6, 0};

const int gamephase[13] = {0, 0, 1, 1, 1, 1, 2, 2, 4, 4, 0, 0, 0};

extern int middlegame[13][64];
extern int endgame[13][64];

void pst_init();

#endif
