#include "move.h"
#include <iostream>
#include <vector>

void print_uci(std::ostream& out, int score, int depth, int nodes, int nps, int time, Move pv[]);
void print_bestmove(std::ostream& out, Move move);
void print_info(std::ostream& out);