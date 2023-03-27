#ifndef PEACEKEEPER_KILLERS
#define PEACEKEEPER_KILLERS

#include "move.h"

struct Killer_table {
    Move table[128][2]{};
    void add(Move move, int ply) {
        if (table[ply][0] != move) {
            table[ply][1] = table[ply][0];
            table[ply][0] = move;
        }
    }
};

#endif
