#include "board.h"
#include "bit_operations.h"
#include "nnue.h"
#include <fstream>

#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"
INCBIN(eval, "./src/default.nn");

std::array<i16, input_size * hidden_size> input_weights;
std::array<i16, hidden_size> input_bias;
std::array<i16, hidden_dsize> hidden_weights;
std::array<i32, output_size> hidden_bias;

template void NNUE::update_accumulator<true>(int piece, int square, int black_king_square, int white_king_square);
template void NNUE::update_accumulator<false>(int piece, int square, int black_king_square, int white_king_square);

template <bool add> void NNUE::update_accumulator(int piece, int square, int black_king_square, int white_king_square) {
    if (piece == empty_square) return;
    Accumulator& accumulator = accumulator_stack[current_accumulator];
    for (int side{}; side < 2; ++side) {
        const int inputs = index(piece, square, side, side ? white_king_square : black_king_square);
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
    i32 output = hidden_bias[0];
    for (int i = 0; i < hidden_size; i++) {
        output += relu(accumulator[side][i]) * hidden_weights[i];
    }
    for (int i = 0; i < hidden_size; i++) {
        output += relu(accumulator[!side][i]) * hidden_weights[hidden_size + i];
    }
    return (output * 400) / input_quantization / hidden_quantization;
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
    fin.read(reinterpret_cast<char*>(hidden_bias.data()), output_size * sizeof(i16));
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