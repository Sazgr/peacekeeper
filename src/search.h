#ifndef PEACEKEEPER_SEARCH
#define PEACEKEEPER_SEARCH

#include "move.h"
#include "nnue.h"

struct Search_stack {
    Move move{};
    Move excluded{};
    int ply{};
    int static_eval{-20001};
    int double_extensions{};
};

struct Search_data {
    u64 nodes{};
    NNUE* nnue = nullptr;
    Move pv_table[128][128];
    Move pv[128];
    int depth_reached;
    int best_score;
};

#endif