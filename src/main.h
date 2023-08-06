#ifndef PEACEKEEPER_MAIN
#define PEACEKEEPER_MAIN

#include "board.h"
#include "hash.h"
#include "order.h"
#include "search.h"
#include "timer.h"
#include <array>
#include <cmath>

#ifdef SPSA
#define spsa
#else
#define spsa constexpr
#endif

enum Features : bool {
    null_move_pruning    = true,
    static_null_move     = true,
    killer_heuristic     = true,
    history_heuristic    = true,
    futility_pruning     = true,
    delta_pruning        = true,
    late_move_pruning    = true,
    late_move_reductions = true,
    check_extensions     = true,
    internal_iterative_reduction = true,
};

spsa std::array<int, 6> futile_margins{47, 75, 98, 119, 138, 156};
constexpr std::array<int, 3> aspiration_bounds{28, 90, 280};
spsa std::array<double, 4> tc_stability{2.05, 1.20, 0.90, 0.85};
constexpr std::array<int, 64> history_bonus{0, 1, 4, 9, 16, 25, 36, 48, 63, 78, 96, 115, 135, 156, 179, 203, 227, 252, 278, 305, 332, 359, 386, 414, 441, 468, 495, 522, 548, 574, 599, 624, 648, 671, 694, 715, 736, 756, 775, 793, 810, 826, 842, 856, 870, 883, 895, 906, 917, 926, 935, 944, 952, 959, 965, 971, 977, 982, 986, 990, 994, 997, 1001, 1003};
constexpr std::array<int, 64> continuation_bonus{0, 1, 4, 9, 16, 24, 34, 45, 57, 70, 83, 97, 111, 124, 138, 150, 163, 174, 184, 194, 203, 211, 218, 224, 230, 234, 238, 242, 245, 247, 249, 251, 252, 253, 254, 254, 255, 255, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256};

spsa double futility_multiplier = 47.5;
spsa double futility_power = 0.666;
spsa double see_noisy_constant = 113.5;
spsa double see_noisy_linear = 0.0;
spsa double see_noisy_quadratic = 8.0;
spsa double see_quiet_constant = 90.0;
spsa double see_quiet_linear = 10.0;
spsa double see_quiet_quadratic = 8.8;
spsa double node_timescale_base = 1.8;
spsa double node_timescale_div = 1.35;
spsa double aspiration_beta_timescale = 1.35;

u64 nodes_used[64][64];

u64 perft(Position& position, int depth);
template <bool side> u64 perft_f(Position& position, int depth);
u64 perft_split(Position& position, int depth, std::vector<std::pair<Move, int>>& list);
bool see(Position& position, Move move, const int threshold);
int quiescence(Position& position, Stop_timer& timer, Hashtable& table, int alpha, int beta, Search_stack* ss);
int pvs(Position& position, Stop_timer& timer, Hashtable& table, Move_order_tables& move_order, int depth, int alpha, int beta, Search_stack* ss);
int iterative_deepening(Position& position, Stop_timer& timer, Hashtable& table, Move_order_tables& move_order, Move& bestmove, bool output);
inline int lmr_reduction(bool is_pv, int depth, int move_num) {
    if (is_pv) return static_cast<int>((std::log(move_num - 3) * std::log(depth)) / 4.3 + 0.5);
    else return static_cast<int>((std::log(move_num - 3) * std::log(depth)) / 1.85 + 0.5);
}
inline int late_move_margin(int depth, int move_num, bool improving) {
    if constexpr (late_move_pruning) {
        if (improving) return ((move_num * move_num) / (depth * depth));
        else return 2 * ((move_num * move_num) / (depth * depth));
    }
    else return 0;
}
inline bool no_mate(int alpha, int beta) {
    return (-18000 < alpha) && (beta < 18000);
}

#endif
