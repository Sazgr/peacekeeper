#ifndef PEACEKEEPER_UCI
#define PEACEKEEPER_UCI

#include "move.h"
#include <iomanip>
#include <iostream>
#include <vector>
#include <fstream>

inline void print_score(std::ostream& out, int score) {
    if (abs(score) <= 18000) out << "cp " << score;
    else if (score < 0) out << "mate -" << (20001 + score) / 2;
    else out << "mate " << (20001 - score) / 2;
}

inline void print_pv(std::ostream& out, Move pv[]) {
    for (int i{}; !pv[i].is_null(); ++i) out << ' ' << pv[i];
}

inline void print_uci(std::ostream& out, int score, int depth, u64 nodes, int nps, int time, Move pv[]) {
    out << "info score ";
    print_score(out, score);
    out << " depth " << depth << " nodes " << nodes << " nps " << nps << " time " << time << " pv";
    print_pv(out, pv);
    out << std::endl;
}

inline void print_info(std::ostream& out) {
    out << "id name Peacekeeper v" << std::fixed << std::setprecision(2) << VERSION << std::defaultfloat << std::setprecision(6) << '\n'
        << "id author Kyle Zhang" << '\n'
        << "option name Hash type spin default 1 min 1 max 1024" << '\n'
        << "option name Threads type spin default 1 min 1 max 1" << '\n'
        << "option name Move Overhead type spin default 5 min 0 max 1000" << '\n'
#ifdef SPSA
        << "option name futility_multiplier type spin default 190 min 0 max 1000" << '\n'
        << "option name futility_power type spin default 67 min 0 max 1000" << '\n'
        << "option name see_noisy_constant type spin default 1135 min 0 max 10000" << '\n'
        << "option name see_noisy_linear type spin default 0 min 0 max 1000" << '\n'
        << "option name see_noisy_quadratic type spin default 50 min 0 max 1000" << '\n'
        << "option name see_quiet_constant type spin default 900 min 0 max 10000" << '\n'
        << "option name see_quiet_linear type spin default 25 min 0 max 1000" << '\n'
        << "option name see_quiet_quadratic type spin default 55 min 0 max 1000" << '\n'
        << "option name node_timescale_base type spin default 180 min 0 max 10000" << '\n'
        << "option name node_timescale_div type spin default 135 min 0 max 1000" << '\n'
        << "option name aspiration_beta_timescale type spin default 135 min 0 max 1000" << '\n'
        << "option name tc_stability_0 type spin default 205 min 0 max 1000" << '\n'
        << "option name tc_stability_1 type spin default 120 min 0 max 1000" << '\n'
        << "option name tc_stability_2 type spin default 90 min 0 max 1000" << '\n'
        << "option name tc_stability_3 type spin default 85 min 0 max 1000" << '\n'
        << "option name nmp_base type spin default 220 min 0 max 1000" << '\n'
        << "option name nmp_depth_div type spin default 360 min 0 max 1000" << '\n'
        << "option name nmp_improving type spin default 100 min 0 max 1000" << '\n'
        << "option name nmp_eval_pow type spin default 30 min 0 max 1000" << '\n'
        << "option name nmp_eval_div type spin default 88 min 0 max 1000" << '\n'
#endif
        << "uciok" << std::endl;
}

inline void print_bestmove(std::ostream& out, Move bestmove) {
    out << "bestmove " << bestmove << std::endl;
}

#endif
