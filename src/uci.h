#ifndef PEACEKEEPER_UCI
#define PEACEKEEPER_UCI

#include "move.h"
#include "options.h"
#include <iomanip>
#include <iostream>
#include <vector>
#include <fstream>

#ifndef VERSION
#define VERSION 0
#endif

inline void print_score(std::ostream& out, int score) {
    if (VERSION == -1 && abs(score) <= 100) {
        out << "cp 0";
        return;
    }
    if (abs(score) <= 18000) out << "cp " << score / 2;
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
    if (VERSION == -1) { //special case for OB makefile compile
        out << "id name Peacekeeper vOB\n";
    } else if (VERSION == 0) {
        out << "id name Peacekeeper vDEV\n";
    } else if (VERSION - (std::round(VERSION * 100) / 100.0) < 0.00000001) { //release version
        out << "id name Peacekeeper v" << std::fixed << std::setprecision(2) << VERSION << std::defaultfloat << std::setprecision(6) << '\n';
    } else { //intermediate version
        out << "id name Peacekeeper v" << std::fixed << std::setprecision(4) << VERSION << std::defaultfloat << std::setprecision(6) << '\n';
    }
    out << "id author Kyle Zhang" << '\n'
        << "option name Hash type spin default 1 min 1 max 1048576" << '\n'
        << "option name Threads type spin default 1 min 1 max 256" << '\n'
        << "option name Move Overhead type spin default 5 min 0 max 1000" << '\n'
        << "option name UCI_Chess960 type check default false" << '\n'
        << "option name EvalFile type string default <internal>" << '\n'
#ifdef SPSA
        << "option name futility_base type spin default 360 min 0 max 1000" << '\n'
        << "option name futility_depth_margin type spin default 870 min 0 max 10000" << '\n'
        << "option name aspiration_base type spin default 280 min 0 max 1000" << '\n'
        << "option name aspiration_power type spin default 316 min 0 max 1000" << '\n'
        << "option name lmr_base type spin default 50 min 0 max 10000" << '\n'
        << "option name lmr_nopv_divisor type spin default 200 min 0 max 10000" << '\n'
        << "option name lmr_ispv_divisor type spin default 400 min 0 max 10000" << '\n'
        << "option name nmp_base type spin default 220 min 0 max 10000" << '\n'
        << "option name nmp_depth_divisor type spin default 400 min 0 max 10000" << '\n'
        << "option name nmp_eval_divisor type spin default 1200 min 0 max 10000" << '\n'
        << "option name see_noisy_constant type spin default 1135 min 0 max 10000" << '\n'
        << "option name see_noisy_linear type spin default 0 min 0 max 1000" << '\n'
        << "option name see_noisy_quadratic type spin default 50 min 0 max 1000" << '\n'
        << "option name see_quiet_constant type spin default 900 min 0 max 10000" << '\n'
        << "option name see_quiet_linear type spin default 25 min 0 max 1000" << '\n'
        << "option name see_quiet_quadratic type spin default 55 min 0 max 1000" << '\n'
        << "option name singular_extension_margin type spin default 64 min 0 max 1000" << '\n'
        << "option name double_extension_margin type spin default 20 min 0 max 1000" << '\n'
        << "option name aspiration_beta_timescale type spin default 135 min 0 max 1000" << '\n'
        << "option name tc_stability_base type spin default 80 min 0 max 1000" << '\n'
        << "option name tc_stability_multiplier type spin default 125 min 0 max 1000" << '\n'
        << "option name tc_stability_power type spin default 313 min 0 max 1000" << '\n'
        << "option name probcut_margin type spin default 200 min 0 max 1000" << '\n'
        << "option name history_pruning_base type spin default 2900 min 0 max 10000" << '\n'
        << "option name history_pruning_depth_margin type spin default 500 min 0 max 10000" << '\n'
        << "option name history_pruning_pv_margin type spin default 500 min 0 max 10000" << '\n'
        << "option name history_pruning_improving_margin type spin default 200 min 0 max 10000" << '\n'
#endif
        << "uciok" << std::endl;
}

inline void print_bestmove(std::ostream& out, Move bestmove) {
    out << "bestmove " << bestmove << std::endl;
}

#endif
