#include "board.h"
#include "bit_operations.h"
#include "fixed_vector.h"
#include "nnue.h"
#include "simd.h"
#include <fstream>

#ifdef _MSC_VER
#define INCBIN_MSVC
#pragma push_macro("_MSC_VER")
#undef _MSC_VER
#endif

#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"

#ifdef INCBIN_MSVC
#pragma pop_macro("_MSC_VER")
#undef INCBIN_MSVC
#endif

INCBIN(eval, NETWORK_FILE);

#ifdef SIMD
alignas(ALIGNMENT) std::array<i16, input_size * hidden_size> input_weights;
alignas(ALIGNMENT) std::array<i16, hidden_size> input_bias;
alignas(ALIGNMENT) std::array<i16, hidden_dsize> hidden_weights;
alignas(ALIGNMENT) std::array<i32, output_size> hidden_bias;
#else
std::array<i16, input_size * hidden_size> input_weights;
std::array<i16, hidden_size> input_bias;
std::array<i16, hidden_dsize> hidden_weights;
std::array<i32, output_size> hidden_bias;
#endif

template void NNUE::update_accumulator<true>(int piece, int square, int black_king_square, int white_king_square);
template void NNUE::update_accumulator<false>(int piece, int square, int black_king_square, int white_king_square);

template <bool add> void NNUE::update_accumulator(int piece, int square, int black_king_square, int white_king_square) {
    Accumulator& accumulator = accumulator_stack[current_accumulator];
    for (int side{}; side < 2; ++side) {
        const int inputs = index(piece, square, side, side ? white_king_square : black_king_square);
#ifdef SIMD
        const register_type* weights = reinterpret_cast<register_type*>(input_weights.data() + inputs * hidden_size);
        const register_type* input = reinterpret_cast<register_type*>(accumulator[side].data());
        register_type* output = reinterpret_cast<register_type*>(accumulator[side].data());
        if constexpr (add) {
            for (int i = 0; i < hidden_size / I16_STRIDE; ++i) {
                output[i] = register_add_16(input[i], weights[i]);
            }
        } else {
            for (int i = 0; i < hidden_size / I16_STRIDE; ++i) {
                output[i] = register_sub_16(input[i], weights[i]);
            }
        }
#else
        const i16* weights = input_weights.data() + inputs * hidden_size;
        if constexpr (add) {
            for (int i = 0; i < hidden_size; ++i) {
                accumulator[side][i] += weights[i];
            }
        } else {
            for (int i = 0; i < hidden_size; ++i) {
                accumulator[side][i] -= weights[i];
            }
        }
#endif
    }
}

void NNUE::update_accumulator_sub_add(std::array<int, 2> sub, std::array<int, 2> add) {
    Accumulator& accumulator = accumulator_stack[current_accumulator];
    for (int side{}; side < 2; ++side) {
#ifdef SIMD
        const register_type* weights_sub = reinterpret_cast<register_type*>(input_weights.data() + sub[side] * hidden_size);
        const register_type* weights_add = reinterpret_cast<register_type*>(input_weights.data() + add[side] * hidden_size);
        const register_type* input = reinterpret_cast<register_type*>(accumulator[side].data());
        register_type* output = reinterpret_cast<register_type*>(accumulator[side].data());
        for (int i = 0; i < hidden_size / I16_STRIDE; ++i) {
            output[i] = register_add_16(register_sub_16(input[i], weights_sub[i]), weights_add[i]);
        }
#else
        const i16* weights_sub = input_weights.data() + sub[side] * hidden_size;
        const i16* weights_add = input_weights.data() + add[side] * hidden_size;
        for (int i = 0; i < hidden_size; ++i) {
            accumulator[side][i] += -weights_sub[i] + weights_add[i];
        }
#endif
    }
}

void NNUE::update_accumulator_sub_sub_add(std::array<int, 2> sub1, std::array<int, 2> sub2, std::array<int, 2> add) {
    Accumulator& accumulator = accumulator_stack[current_accumulator];
    for (int side{}; side < 2; ++side) {
#ifdef SIMD
        const register_type* weights_sub1 = reinterpret_cast<register_type*>(input_weights.data() + sub1[side] * hidden_size);
        const register_type* weights_sub2 = reinterpret_cast<register_type*>(input_weights.data() + sub2[side] * hidden_size);
        const register_type* weights_add = reinterpret_cast<register_type*>(input_weights.data() + add[side] * hidden_size);
        const register_type* input = reinterpret_cast<register_type*>(accumulator[side].data());
        register_type* output = reinterpret_cast<register_type*>(accumulator[side].data());
        for (int i = 0; i < hidden_size / I16_STRIDE; ++i) {
            output[i] = register_add_16(register_sub_16(register_sub_16(input[i], weights_sub1[i]), weights_sub2[i]), weights_add[i]);
        }
#else
        const i16* weights_sub1 = input_weights.data() + sub1[side] * hidden_size;
        const i16* weights_sub2 = input_weights.data() + sub2[side] * hidden_size;
        const i16* weights_add = input_weights.data() + add[side] * hidden_size;
        for (int i = 0; i < hidden_size; ++i) {
            accumulator[side][i] += -weights_sub1[i] - weights_sub2[i] + weights_add[i];
        }
#endif
    }
}

void NNUE::refresh(Position& position) {
    Accumulator &accumulator = accumulator_stack[current_accumulator];
    accumulator.clear();
    u64 pieces = position.occupied;
    const int black_king_square = get_lsb(position.pieces[black_king]);
    const int white_king_square = get_lsb(position.pieces[white_king]);
    while (pieces) {
        int square = pop_lsb(pieces);
        int piece = position.board[square];
        update_accumulator<true>(piece, square, black_king_square, white_king_square);
    }
}

i32 NNUE::evaluate(bool side) {
    Accumulator &accumulator = accumulator_stack[current_accumulator];
#ifdef SIMD
    const register_type screlu_min{};
    const register_type screlu_max = register_set_16(input_quantization);
    register_type res{};
    const register_type* accumulator_us = reinterpret_cast<register_type*>(accumulator[side].data());
    const register_type* accumulator_them = reinterpret_cast<register_type*>(accumulator[!side].data());
    const register_type* weights = reinterpret_cast<register_type*>(hidden_weights.data());
    for (int i = 0; i < hidden_size / I16_STRIDE; ++i) {
        const register_type clipped = register_min_16(register_max_16(accumulator_us[i], screlu_min), screlu_max);
        res = register_add_32(res, register_madd_16(register_mul_16(clipped, clipped), weights[i]));
    }
    for (int i = 0; i < hidden_size / I16_STRIDE; ++i) {
        const register_type clipped = register_min_16(register_max_16(accumulator_them[i], screlu_min), screlu_max);
        res = register_add_32(res, register_madd_16(register_mul_16(clipped, clipped), weights[i + hidden_size / I16_STRIDE]));
    }
    i32 output = register_sum_32(res) + (hidden_bias[0] * input_quantization);
#else
    i32 output = hidden_bias[0] * input_quantization;
    for (int i = 0; i < hidden_size; ++i) {
        output += screlu(accumulator[side][i]) * hidden_weights[i];
    }
    for (int i = 0; i < hidden_size; ++i) {
        output += screlu(accumulator[!side][i]) * hidden_weights[hidden_size + i];
    }
#endif
    return (output / input_quantization * 400) / input_quantization / hidden_quantization;
}

void load_default() {
    uint64_t memory_index = 0;
    std::memcpy(input_weights.data(), &geval_data[memory_index], input_size * hidden_size * sizeof(i16));
    memory_index += input_size * hidden_size * sizeof(i16);
    std::memcpy(input_bias.data(), &geval_data[memory_index], hidden_size * sizeof(i16));
    memory_index += hidden_size * sizeof(i16);
    std::memcpy(hidden_weights.data(), &geval_data[memory_index], hidden_dsize * output_size * sizeof(i16));
    memory_index += hidden_dsize * output_size * sizeof(i16);
    std::memcpy(hidden_bias.data(), &geval_data[memory_index], output_size * sizeof(i32));
    memory_index += output_size * sizeof(i32);
}

void load_from_file(std::string& name) {
    std::ifstream fin(name, std::ios::binary);
    fin.read(reinterpret_cast<char*>(input_weights.data()), input_size * hidden_size * sizeof(i16));
    fin.read(reinterpret_cast<char*>(input_bias.data()), hidden_size * sizeof(i16));
    fin.read(reinterpret_cast<char*>(hidden_weights.data()), hidden_dsize * output_size * sizeof(i16));
    fin.read(reinterpret_cast<char*>(hidden_bias.data()), output_size * sizeof(i32));
    if (!fin) {
        std::cout << "info string error could not load net from " << name << std::endl;
        load_default();
    } else {
        std::cout << "info string loaded net from " << name << std::endl;
    }
}

void nnue_init() {
    load_default();
}