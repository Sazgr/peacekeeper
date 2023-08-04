#ifndef PEACEKEEPER_SEARCH
#define PEACEKEEPER_SEARCH

struct Search_stack {
    Move move{};
    Move excluded{};
    int ply{};
    int static_eval{-20001};
};

#endif