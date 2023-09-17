#ifndef PEACEKEEPER_SEARCH
#define PEACEKEEPER_SEARCH

#include "nnue.h"

struct Search_stack {
    Move move{};
    Move excluded{};
    int ply{};
    int static_eval{-20001};
};

struct Search_data {
    u64 nodes{};
    NNUE* nnue = nullptr;
};

#endif