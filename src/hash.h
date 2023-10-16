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

//how an entry is packed
//6666555555555544444444443333333333222222222211111111110000000000
//3210987654321098765432109876543210987654321098765432109876543210
//age-----depth---type----score-----------move--------------------

struct Packed_element;
struct Element;

struct Packed_element {
    u64 full_hash;
    u64 data;
    Packed_element() {
        data = (static_cast<u64>(tt_none) << 40) | (Move{}.data & 0xFFFFFF);
    }
    Packed_element(u64 hash, Move move, int sc, u8 tp, u8 dp, u8 ag, int ply) {
        u64 full_hash_to_store = hash;
	if (sc < -18000) sc += ply;
	if (sc > 18000) sc -= ply;
        u64 data_to_store = (static_cast<u64>(ag) << 56) | (static_cast<u64>(dp) << 48) | (static_cast<u64>(tp) << 40) | (static_cast<u64>(static_cast<u16>(static_cast<i16>(sc))) << 24) | (move.data & 0xFFFFFF);
        full_hash_to_store ^= data_to_store; //xor trick
        data = data_to_store;
        full_hash = full_hash_to_store;
    }
    Packed_element& operator=(const Packed_element& rhs) {
        full_hash = rhs.full_hash;
        data = rhs.data;
        return *this;
    }
};

struct Element {
    u64 full_hash;
    Move bestmove;
    int score;
    int type;
    int depth;
    int age;
    Element() {
        bestmove = Move{};
        type = tt_none;
    }
    Element(u64 hash, Move move, int sc, u8 tp, u8 dp, u8 ag, int ply) {
        full_hash = hash;
        bestmove = move;
        score = sc;
	if (score < -18000) score += ply;
	if (score > 18000) score -= ply;
        type = tp;
        depth = dp;
        age = ag;
    }
    Element& operator=(const Element& rhs) {
        full_hash = rhs.full_hash;
        bestmove = rhs.bestmove;
        score = rhs.score;
        type = rhs.type;
        depth = rhs.depth;
        age = rhs.age;
        return *this;
    }
    Element(Packed_element& rhs) {
        Packed_element copy = rhs;
        copy.full_hash ^= copy.data;
        full_hash = copy.full_hash;
        bestmove = Move{copy.data & 0x0000000000FFFFFF};
        score = static_cast<i16>(static_cast<u16>((copy.data >> 24) & 0xFFFF));
        type = (copy.data >> 40) & 0xFF;
        depth = (copy.data >> 48) & 0xFF;
        age = (copy.data >> 56) & 0xFF;
    }
    inline Element& adjust_score(int ply) {
        if (score < -18000) score -= ply;
        if (score > 18000) score += ply;
        return *this;
    }
};

class Hashtable {
public:
    explicit Hashtable(u64 sz) {
        size = 1ull << get_msb((sz * 1048576) / 16); //converting mb to size
        table.resize(size);
    }
    void resize(u64 sz) {
        size = 1ull << get_msb((sz * 1048576) / 16); //converting mb to size
        table.resize(size);
    }
    void prefetch(const u64 hash) const {
        __builtin_prefetch(&table[hash & (size - 1)]);
    }
    Element query(const u64 hash) {
        return Element(table[hash & (size - 1)]);
    }
    void clear() {
        for (int i{0}; i<size; ++i) table[i] = Packed_element{};
    }
    void age() {
        ; //nothing needed here
    }
    void insert(const u64 hash, int score, u8 type, Move bestmove, u8 dp, int ply) {
        Element previous = Element(table[hash & (size - 1)]);
        if (bestmove.is_null() && previous.full_hash == hash) {
            bestmove = previous.bestmove;
        }
        if (previous.depth <= dp + 3) table[hash & (size - 1)] = Packed_element{hash, bestmove, score, type, dp, table_age, ply};
    }
private:
    std::vector<Packed_element> table;
    u64 size;
    u8 table_age;
};

#endif
