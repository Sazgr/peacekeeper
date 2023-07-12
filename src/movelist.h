#ifndef PEACEKEEPER_MOVELIST
#define PEACEKEEPER_MOVELIST

#include "move.h"
#include <algorithm>

class Movelist {
    Move list[220];
    int count{};
public:
    inline int size() {return count;}
    inline void add(Move move) {list[count++] = move;}
    inline void clear() {count = 0;}
    inline Move& operator[](int index) {return list[index];}
    inline void sort(int start, int end) {std::sort(std::begin(list) + start, std::begin(list) + end);}
    inline void pick(int start) {
        int best = start;
        for (int i{start + 1}; i < size(); ++i) {
            if (list[i] < list[best]) {
                best = i;
            }
        }
        std::swap(list[start], list[best]);
    }
};

#endif
