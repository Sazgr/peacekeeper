#ifndef PEACEKEEPER_HASH
#define PEACEKEEPER_HASH

#include "bit_operations.h"
#include "board.h"
#include "typedefs.h"
#include <vector>
#include <cstdint>

enum Transposition_entry_types {
    tt_exact,
    tt_alpha,
    tt_beta,
    tt_none
};

struct Element {
    u64 full_hash;                  //8
    Move bestmove;                  //8
    int score;                      //4
    std::uint8_t type{tt_none}; //1
    std::uint8_t depth;             //1
    Element(u64 hash, Move move, int sc, std::uint8_t tp, std::uint8_t dp) {
        full_hash = hash;
        bestmove = move;
        score = sc;
        type = tp;
        depth = dp;
    }
    Element() {
        type = tt_none;
    }
};

class Hashtable {
public:
    explicit Hashtable(u64 sz) {
        size = 1ull << get_msb((sz * 1048576) / 24); //converting mb to size
        table.resize(size);
    }
    void resize(u64 sz) {
        size = 1ull << get_msb((sz * 1048576) / 24); //converting mb to size
        table.resize(size);
    }
    void prefetch(const u64 hash) const {
        __builtin_prefetch(&table[hash & (size-1)]);
    }
    Element& query(const u64 hash) {
        return table[hash & (size-1)];
    }
    void clear() {
        for (int i{0}; i<size; ++i) table[i].type = tt_none;
    }
    void insert(const u64 hash, int score, std::uint8_t type, Move bestmove, std::uint8_t dp) {
        if (table[hash & (size-1)].type == tt_none || table[hash & (size-1)].full_hash != hash || table[hash & (size-1)].depth < dp || table[hash & (size-1)].type != tt_exact || type == tt_exact)
            table[hash & (size-1)] = Element{hash, bestmove, score, type, dp};
    }
private:
    std::vector<Element> table;
    u64 size;
};

#endif
