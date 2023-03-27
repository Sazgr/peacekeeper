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
    u32 bestmove;                   //4
    int score;                      //4
    std::uint8_t type{tt_none}; //1
    std::uint8_t depth;             //1
    std::uint8_t age;               //1
    Element(u64 hash, Move move, int sc, std::uint8_t tp, std::uint8_t dp, std::uint8_t ag) {
        full_hash = hash;
        bestmove = move;
        score = sc;
        type = tp;
        depth = dp;
        age = ag;
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
        if (table[(hash & (size - 2))].full_hash == hash) {
            table[(hash & (size - 2))].age = table_age;
            return table[(hash & (size - 2))];
        } else {
            table[(hash & (size - 2)) + 1].age = table_age;
            return table[(hash & (size - 2)) + 1];
        }
    }
    void clear() {
        for (int i{0}; i<size; ++i) table[i].type = tt_none;
    }
    void age() {
        ++table_age;
    }
    int index(const u64 hash, int score, std::uint8_t type, Move bestmove, std::uint8_t dp) {
        if (table[(hash & (size - 2))].full_hash == hash) return 0; //first test hashes to prevent storing any duplicate positions
        if (table[(hash & (size - 2)) + 1].full_hash == hash) return 1;
        if (table[(hash & (size - 2))].type == tt_none) return 0; //then check if a slot is empty
        if (table[(hash & (size - 2)) + 1].type == tt_none) return 1;
        if (table[(hash & (size - 2))].depth < dp || table[(hash & (size - 2))].age != table_age) return 0; //now if we have a deeper search or an aged search in the first slot, we replace it
        return 1;
    }
    void insert(const u64 hash, int score, std::uint8_t type, Move bestmove, std::uint8_t dp) {
        int offset = index(hash, score, type, bestmove, dp);
        table[(hash & (size - 2)) + offset] = Element{hash, bestmove, score, type, dp, table_age};
    }
private:
    std::vector<Element> table;
    u64 size;
    std::uint8_t table_age;
};

#endif
