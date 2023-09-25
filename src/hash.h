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

struct Element {
    u64 full_hash;
    u64 data;
    Element(u64 hash, Move move, int sc, u8 tp, u8 dp, u8 ag, int ply) {
	if (sc < -18000) sc += ply;
	if (sc > 18000) sc -= ply;
        u64 packed_data = (static_cast<u64>(ag) << 56) | (static_cast<u64>(dp) << 48) | (static_cast<u64>(tp) << 40) | (static_cast<u64>(static_cast<u16>(static_cast<i16>(sc))) << 24) | (move.data & 0xFFFFFF);
        full_hash = hash ^ packed_data;
        data = packed_data;
    }
    Element() {
        data = (static_cast<u64>(tt_none) << 40) | (Move{}.data & 0xFFFFFF);
    }
    Element& operator=(const Element& rhs) {
        u64 packed_data = ((!rhs.bestmove().is_null() || full_hash != rhs.full_hash) ? rhs.data : (data & 0x0000000000FFFFFF) | (rhs.data & 0xFFFFFFFFFF000000));
        full_hash = rhs.full_hash ^ packed_data;
        data = packed_data;
        return *this;
    }
    inline void set_age(u8 new_age) {
        u64 packed_data = (data & 0x00FFFFFFFFFFFFFF) | (static_cast<u64>(new_age) << 56);
        u64 hash = full_hash ^ packed_data;
        full_hash = hash ^ packed_data;
        data = packed_data;
    }
    inline const Move bestmove() const {
        return Move{data & 0x0000000000FFFFFF};
    }
    inline Element& adjust_score(int ply) {
        if (score() < -18000) data = (data & 0xFFFFFF0000FFFFFF) | (static_cast<u64>(static_cast<u16>(static_cast<i16>(score() - ply))) << 24);
        if (score() > 18000) data = (data & 0xFFFFFF0000FFFFFF) | (static_cast<u64>(static_cast<u16>(static_cast<i16>(score() + ply))) << 24);
        return *this;
    }
    inline constexpr u64 hash() const {
        return full_hash ^ data;
    }
    inline constexpr int score() const {
        return static_cast<i16>(static_cast<u16>((data >> 24) & 0xFFFF));
    }
    inline constexpr int type() const {
        return (data >> 40) & 0xFF;
    }
    inline constexpr int depth() const {
        return (data >> 48) & 0xFF;
    }
    inline constexpr u8 age() const {
        return (data >> 56) & 0xFF;
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
        __builtin_prefetch(&table[hash & (size - 2)]);
        __builtin_prefetch(&table[(hash & (size - 2)) + 1]);
    }
    Element query(const u64 hash) {
        Element entry{};
        if (table[(hash & (size - 2))].hash() == hash) {
            table[(hash & (size - 2))].set_age(table_age);
            entry = table[(hash & (size - 2))];
        } else {
            table[(hash & (size - 2)) + 1].set_age(table_age);
            entry = table[(hash & (size - 2)) + 1];
        }
        if (entry.hash() != hash) return Element{};
        return entry;
    }
    void clear() {
        for (int i{0}; i<size; ++i) table[i] = Element{};
    }
    void age() {
        ++table_age;
    }
    int index(const u64 hash, int score, u8 type, Move bestmove, u8 dp) {
        if (table[(hash & (size - 2))].hash() == hash) return 0; //first test hashes to prevent storing any duplicate positions
        if (table[(hash & (size - 2)) + 1].hash() == hash) return 1;
        if (table[(hash & (size - 2))].type() == tt_none) return 0; //then check if a slot is empty
        if (table[(hash & (size - 2)) + 1].type() == tt_none) return 1;
        if (table[(hash & (size - 2))].depth() < dp || table[(hash & (size - 2))].age() != table_age) return 0; //now if we have a deeper search or an aged search in the first slot, we replace it
        return 1;
    }
    void insert(const u64 hash, int score, u8 type, Move bestmove, u8 dp, int ply) {
        int offset = index(hash, score, type, bestmove, dp);
        table[(hash & (size - 2)) + offset] = Element{hash, bestmove, score, type, dp, table_age, ply};
    }
private:
    std::vector<Element> table;
    u64 size;
    u8 table_age;
};

#endif
