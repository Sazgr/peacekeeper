#ifndef PEACEKEEPER_SEARCH
#define PEACEKEEPER_SEARCH

#include "nnue.h"

struct Search_stack {
    int static_eval{-20001};
    Move move{};
    int ply{};
};

struct Search_data {
    u64 nodes{};
    NNUE* nnue = nullptr;
};

#endif