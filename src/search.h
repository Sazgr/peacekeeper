#ifndef PEACEKEEPER_SEARCH
#define PEACEKEEPER_SEARCH

struct Search_stack {
    int static_eval{-20001};
    Move move{};
    int ply{};
};

#endif