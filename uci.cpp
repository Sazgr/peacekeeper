#include "uci.h"

#include <fstream>

void print_score(std::ostream& out, int score) {
    if (abs(score) <= 18000) {
        out << "cp " << score;
    } else if (score < 0) {
        out << "mate -" << (20001 + score) / 2;
    } else {
        out << "mate " << (20001 - score) / 2;
    }
}
void print_pv(std::ostream& out, Move pv[]) {
    for (int i{}; !pv[i].is_null(); ++i) {
        out << ' ' << pv[i];
    }
}
void print_uci(std::ostream& out, int score, int depth, int nodes, int nps, int time, Move pv[]) {
    out << "info score ";
    print_score(out, score);
    out << " depth " << depth << " nodes " << nodes << " nps " << nps << " time " << time << " pv";
    print_pv(out, pv);
    out << std::endl;
}

void print_info(std::ostream& out) {
    out << "id name Peacekeeper v0.10" << '\n'
              << "id author Kyle Zhang" << '\n'
              << "option name Hash type spin default 1 min 1 max 128" << '\n'
              << "uciok" << std::endl;
}

void print_bestmove(std::ostream& out, Move bestmove) {
    out << "bestmove " << bestmove << std::endl;
}