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
    Element(u64 hash, Move move, int sc, u8 tp, u8 dp, u8 ag) {
        full_hash = hash;
        data = (static_cast<u64>(ag) << 56) | (static_cast<u64>(dp) << 48) | (static_cast<u64>(tp) << 40) | (static_cast<u64>(static_cast<u32>(sc)) << 24) | (move.data & 0xFFFFFF);
    }
    Element() {
        data = static_cast<u64>(tt_none) << 40;
    }
    Element& operator=(const Element& rhs) {
        if (!rhs.bestmove().is_null() || full_hash != rhs.full_hash) {
            data = rhs.data;
        } else {
            data = (data & 0x0000000000FFFFFF) | (rhs.data & 0xFFFFFFFFFF000000);
        }
        return *this;
    }
    inline void set_age(int new_age) {
        data = (data & 0x00FFFFFFFFFFFFFF) | (static_cast<u64>(new_age) << 56);
    }
    inline const Move bestmove() const {
        return Move{data & 0x0000000000FFFFFF};
    }
    inline constexpr int score() const {
        return (data & 0x000000FFFF000000) >> 24;
    }
    inline constexpr int type() const {
        return (data & 0x0000FF0000000000) >> 40;
    }
    inline constexpr int depth() const {
        return (data & 0x00FF000000000000) >> 48;
    }
    inline constexpr int age() const {
        return data >> 56;
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
        __builtin_prefetch(&table[hash & (size - 2)]);
        __builtin_prefetch(&table[(hash & (size - 2)) + 1]);
    }
    Element& query(const u64 hash) {
        if (table[(hash & (size - 2))].full_hash == hash) {
            table[(hash & (size - 2))].set_age(table_age);
            return table[(hash & (size - 2))];
        } else {
            table[(hash & (size - 2)) + 1].set_age(table_age);
            return table[(hash & (size - 2)) + 1];
        }
    }
    void clear() {
        for (int i{0}; i<size; ++i) table[i] = Element{};
    }
    void age() {
        ++table_age;
    }
    int index(const u64 hash, int score, std::uint8_t type, Move bestmove, std::uint8_t dp) {
        if (table[(hash & (size - 2))].full_hash == hash) return 0; //first test hashes to prevent storing any duplicate positions
        if (table[(hash & (size - 2)) + 1].full_hash == hash) return 1;
        if (table[(hash & (size - 2))].type() == tt_none) return 0; //then check if a slot is empty
        if (table[(hash & (size - 2)) + 1].type() == tt_none) return 1;
        if (table[(hash & (size - 2))].depth() < dp || table[(hash & (size - 2))].age() != table_age) return 0; //now if we have a deeper search or an aged search in the first slot, we replace it
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
