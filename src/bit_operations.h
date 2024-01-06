#ifndef PEACEKEEPER_BIT_OPERATIONS
#define PEACEKEEPER_BIT_OPERATIONS

#include "typedefs.h"

#if defined(__GNUC__) || defined(__clang__)

inline int popcount(u64 bits) {return __builtin_popcountll(bits);}
inline int get_lsb(u64 bits) {return __builtin_ctzll(bits);}
inline int get_msb(u64 bits) {return __builtin_clzll(bits) ^ 63;}
inline int bitscan(u64 bits, bool reverse) {
    u64 rmask;
    rmask = -u64(reverse);
    bits &= -bits | rmask;
    return get_msb(bits);
}
inline int pop_lsb(u64& bits) {
    int lsb = get_lsb(bits);
    bits &= bits - 1;
    return lsb;
}

#elif defined(_MSC_VER)

#include <intrin.h>

inline int popcount(u64 bits) {return __popcnt64(bits);}
inline int get_lsb(u64 bits) {return _BitScanForward64(bits);}
inline int get_msb(u64 bits) {return _BitScanReverse64(bits);}
inline int bitscan(u64 bits, bool reverse) {
    u64 rmask;
    rmask = -u64(reverse);
    bits &= -bits | rmask;
    return get_msb(bits);
}
inline int pop_lsb(u64& bits) {
    int lsb = get_lsb(bits);
    __blsr_u64(bits);
    return lsb;
}

#else

#include <bit>

inline int popcount(u64 bits) {return std::popcount(bits);}
inline int get_lsb(u64 bits) {return std::countr_zero(bits);}
inline int get_msb(u64 bits) {return std::countl_zero(bits) ^ 63;}
inline int bitscan(u64 bits, bool reverse) {
    u64 rmask;
    rmask = -u64(reverse);
    bits &= -bits | rmask;
    return get_msb(bits);
}
inline int pop_lsb(u64& bits) {
    int lsb = get_lsb(bits);
    bits &= bits - 1;
    return lsb;
}

#endif

#endif
