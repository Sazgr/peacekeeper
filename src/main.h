#ifndef PEACEKEEPER_MAIN
#define PEACEKEEPER_MAIN

#include "board.h"
#include "hash.h"
#include "history.h"
#include "timer.h"
#include <array>
#include <cmath>

enum Features : bool {
    null_move_pruning    = true,
    static_null_move     = true,
    history_heuristic    = true,
    futility_pruning     = true,
    delta_pruning        = true,
    late_move_reductions = true,
    check_extensions     = true,
};

std::array<int, 4> futile_margins{145, 205, 251, 290};

u64 perft(Position& position, int depth);
template <bool side> u64 perft_f(Position& position, int depth);
u64 perft_split(Position& position, int depth, std::vector<std::pair<Move, int>>& list);
int quiescence(Position& position, Stop_timer& timer, int ply, int alpha, int beta);
int pvs(Position& position, Stop_timer& timer, Hashtable& table, History_table& history, int depth, int ply, int alpha, int beta, bool is_pv, bool can_null);
void iterative_deepening(Position& position, Stop_timer& timer, Hashtable& table, History_table& history, Move& bestmove);
inline int lmr_reduction(bool is_pv, int depth, int move_num) {
    if (is_pv) return static_cast<int>((std::log(move_num - 2) * std::log(depth) + 1.44) / 4.1);
    else return static_cast<int>((std::log(move_num - 2) * std::log(depth) + 0.9) / 1.9);
}
inline bool no_mate(int alpha, int beta) {
    return (-18000 < alpha) && (beta < 18000);
}

#endif
