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
#ifdef DATAGEN
    late_move_pruning    = false,
#else
    late_move_pruning    = true,
#endif
    late_move_reductions = true,
    check_extensions     = true,
    internal_iterative_reduction = true,
    razoring             = true,
};

spsa std::array<int, 3> aspiration_bounds{28, 90, 280};
spsa std::array<double, 4> tc_stability{2.05, 1.20, 0.90, 0.85};

spsa double futility_base = 36;
spsa double futility_depth_margin = 87;
spsa double aspiration_base = 28;
spsa double aspiration_power = 3.16;
spsa double lmr_base = 0.5;
spsa double lmr_nopv_divisor = 2.0;
spsa double lmr_ispv_divisor = 4.0;
spsa double nmp_base = 2.19999999;
spsa double nmp_depth_divisor = 4.0;
spsa double nmp_eval_divisor = 12.0;
spsa double see_noisy_constant = 290.6;
spsa double see_noisy_linear = 0.0;
spsa double see_noisy_quadratic = 20.5;
spsa double see_quiet_constant = 230.4;
spsa double see_quiet_linear = 25.6;
spsa double see_quiet_quadratic = 22.5;
spsa int singular_extension_margin = 64;
spsa int double_extension_margin = 20;
spsa double aspiration_beta_timescale = 1.35;
spsa double tc_stability_base = 0.80;
spsa double tc_stability_multiplier = 1.25;
spsa double tc_stability_power = 0.313;

int lmr_reduction_table[2][64][220];

u64 perft(Position& position, int depth);
template <bool side> u64 perft_f(Position& position, int depth);
u64 perft_split(Position& position, int depth, std::vector<std::pair<Move, int>>& list);
void datagen_thread(int thread_id, std::string out_base, int soft_nodes_limit);
bool see(Position& position, Move move, const int threshold);
int quiescence(Position& position, Stop_timer& timer, Hashtable& table, int alpha, int beta, Search_stack* ss, Search_data& sd);
int pvs(Position& position, Stop_timer& timer, Hashtable& table, Move_order_tables& move_order, int depth, int alpha, int beta, Search_stack* ss, Search_data& sd, bool cutnode);
void iterative_deepening(Position& position, Stop_timer& timer, Hashtable& table, Move_order_tables& move_order, Move& bestmove, Search_data& sd, bool output);
int iterative_deepening_base(Position& position, Stop_timer& timer, Hashtable& table, Move_order_tables& move_order, Move& bestmove, Search_data& sd, bool output);
inline void fill_lmr_reduction_table() {
    for (int depth{1}; depth < 64; ++depth) {
        for (int move_num{3}; move_num < 220; ++move_num) {
            lmr_reduction_table[0][depth][move_num] = static_cast<int>((std::log(move_num - 2) * std::log(depth)) / lmr_nopv_divisor + lmr_base);
            lmr_reduction_table[1][depth][move_num] = static_cast<int>((std::log(move_num - 2) * std::log(depth)) / lmr_ispv_divisor + lmr_base);
        }
    }
}
inline int lmr_reduction(bool is_pv, int depth, int move_num) {
    return lmr_reduction_table[is_pv][depth][move_num];
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
