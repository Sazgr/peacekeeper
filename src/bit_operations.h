#ifndef PEACEKEEPER_BIT_OPERATIONS
#define PEACEKEEPER_BIT_OPERATIONS

#include "typedefs.h"

inline int popcount(u64 bits) {
#ifdef __GNUC__
    return __builtin_popcountll(bits);
#elif _MSC_VER
    return __popcnt64(bits);
#endif
}

inline int get_lsb(u64 bits) {
#ifdef __GNUC__
    return __builtin_ctzll(bits);
#elif _MSC_VER
    return _BitScanForward64(bits);
#endif
}

inline int get_msb(u64 bits) {
#ifdef __GNUC__
    return __builtin_clzll(bits) ^ 63;
#elif _MSC_VER
    return _BitScanReverse64(bits);
#endif
}

inline int bitscan(u64 bits, bool reverse) {
    u64 rmask;
    rmask = -u64(reverse);
    bits &= -bits | rmask;
    return get_msb(bits);
}

inline int pop_lsb(u64& bits) {
    int lsb = get_lsb(bits);
#ifdef _MSC_VER
    __blsr_u64(bits);
#else
    bits &= bits - 1;
#endif
    return lsb;
}

#endif
