#ifndef PEACEKEEPER_FIXED_VECTOR
#define PEACEKEEPER_FIXED_VECTOR

#include "move.h"
#include <algorithm>

template <typename T, u64 capacity>
class Fixed_vector {
    T list[capacity];
    int count{};
public:
    inline int size() {return count;}
    inline void add(T item) {list[count++] = item;}
    inline void clear() {count = 0;}
    inline Move& operator[](int index) {return list[index];}
    inline void sort(int start, int end) {std::sort(std::begin(list) + start, std::begin(list) + end);}
};

using Movelist = Fixed_vector<Move, 220>;

#endif
