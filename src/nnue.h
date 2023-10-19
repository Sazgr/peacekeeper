#ifndef PEACEKEEPER_NNUE
#define PEACEKEEPER_NNUE

#include "simd.h"
#include "typedefs.h"
#include <array>
#include <cstring>

constexpr int buckets = 1;
constexpr int input_size = 12 * 64 * buckets;
constexpr int hidden_size = 768;
constexpr int hidden_dsize = hidden_size * 2;
constexpr int output_size = 1;
constexpr int input_quantization = 255;
constexpr int hidden_quantization = 64;

extern std::array<i16, input_size * hidden_size> input_weights;
extern std::array<i16, hidden_size> input_bias;
extern std::array<i16, hidden_dsize> hidden_weights;
extern std::array<i32, output_size> hidden_bias;

const int king_buckets[64] {
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    2, 2, 3, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 2, 2,
};

static inline int king_bucket(int king_square, bool king_color) {
    if constexpr (buckets > 1) {
        king_square ^= 56;
        king_square = (56 * king_color) ^ king_square;
        return king_buckets[king_square];
    } else {
        return 0;
    }
}

static inline int index(int piece, int square, bool view, int king_square) {
    square ^= 56;
    constexpr u32 ColorStride = 64 * 6;
    constexpr u32 PieceStride = 64;
    const auto type = static_cast<u32>(piece / 2);
    const u32 color = (piece & 1) ^ 1;

    return (view ? (color * ColorStride + type * PieceStride +  static_cast<u32>(square)) : (!color * ColorStride + type * PieceStride + (static_cast<u32>(square) ^ 0x38)));
}

static inline i16 crelu(i16 input) {
    return std::clamp<i16>(input, 0, input_quantization);
}

struct Accumulator {
#ifdef SIMD
    alignas(ALIGNMENT) std::array<i16, hidden_size> black;
    alignas(ALIGNMENT) std::array<i16, hidden_size> white;
#else
    std::array<i16, hidden_size> black;
    std::array<i16, hidden_size> white;
#endif
    std::array<i16, hidden_size>& operator[](bool side) {
        return side ? white : black;
    }
    inline void clear() {
        white = input_bias;
        black = input_bias;
    }
};

class Position;

class NNUE {
    i32 current_accumulator = 0;
    std::array<Accumulator, 128> accumulator_stack;
public:
    NNUE() {
        for (int i{}; i < 128; ++i) {
            accumulator_stack[i] = Accumulator();
        }
    }
    inline void push() {
        accumulator_stack[current_accumulator + 1] = accumulator_stack[current_accumulator];
        ++current_accumulator;
    }
    inline void pop() { 
        --current_accumulator; 
    }
    inline void reset_accumulators() { 
        current_accumulator = 0;
    }
    void refresh(Position& position);
    template <bool add> void update_accumulator(int piece, int square, int black_king_square, int white_king_square);
    i32 evaluate(bool side);
};

void load_default();
void load_from_file(std::string& name);
void nnue_init();

#endif