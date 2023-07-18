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
    if (is_pv) return static_cast<int>((std::log(move_num) * std::log(depth)) / 4.4 + 0.5);
    else return static_cast<int>((std::log(move_num) * std::log(depth)) / 1.9 + 0.5);
}
inline int late_move_margin(int depth, int move_num) {
    if constexpr (late_move_pruning) return (move_num * move_num) / (depth * depth);
    else return 0;
}
inline bool no_mate(int alpha, int beta) {
    return (-18000 < alpha) && (beta < 18000);
}

#endif
