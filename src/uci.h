#ifndef PEACEKEEPER_UCI
#define PEACEKEEPER_UCI

#include "move.h"
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
        << "option name Move Overhead type spin default 20 min 0 max 500" << '\n'
        << "uciok" << std::endl;
}

inline void print_bestmove(std::ostream& out, Move bestmove) {
    out << "bestmove " << bestmove << std::endl;
}

#endif
