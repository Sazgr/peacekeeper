#ifndef PEACEKEEPER_BOARD
#define PEACEKEEPER_BOARD

#include "eval.h"
#include "lookup_arrays.h"
#include "magics.h"
#include "move.h"
#include "movelist.h"
#include "typedefs.h"
#include <cstdint>
#include <string>
#include <vector>

enum Piece_types {
    black_pawn,
    white_pawn,
    black_knight,
    white_knight,
    black_bishop,
    white_bishop,
    black_rook,
    white_rook,
    black_queen,
    white_queen,
    black_king,
    white_king,
    empty_square
};

enum Move_types {
    quiet = 1,
    noisy,
    all
};

const int castling_disable[64] {
    14, 15, 15, 15, 12, 15, 15, 13,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    11, 15, 15, 15,  3, 15, 15,  7
};

class Position {
private: //simple templates
    template <bool side> inline u64 pawns_forward_one(u64 pawns);
    template <bool side> inline u64 pawns_backward_one(u64 pawns);
    template <bool side> inline u64 pawns_forward_two(u64 pawns);
    template <bool side> inline u64 pawns_backward_two(u64 pawns);
    template <bool side> inline u64 pawns_forward_left(u64 pawns);
    template <bool side> inline u64 pawns_backward_left(u64 pawns);
    template <bool side> inline u64 pawns_forward_right(u64 pawns);
    template <bool side> inline u64 pawns_backward_right(u64 pawns);
    template <bool side> inline u64 promotion_rank();
    template <bool side> inline u64 second_rank();
public:
    template <bool side_to_attack> u64 attack_map(u64 occupied);
    bool check();
    template <Move_types types, bool side> void gen_legal(Movelist& movelist);
    template <Move_types types, bool side> u64 count_legal();
    void legal_moves(Movelist& movelist);
    void legal_noisy(Movelist& movelist);
    void legal_quiet(Movelist& movelist);
    u64 count_legal_moves();
    void print_bitboard(u64 bits);
    void print(std::ostream& out);
    void make_move(Move move);
    void undo_move(Move move);
    inline void make_null();
    inline void undo_null();
    template <bool update_hash> inline void fill_sq(int sq, int piece);
    void load(std::vector<int> position, bool stm = true);
    std::string export_fen();
    bool load_fen(std::string fen_pos, std::string fen_stm, std::string fen_castling, std::string fen_ep, std::string fen_hmove_clock, std::string fen_fmove_counter);
    bool parse_move(Move& out, std::string move);
    int static_eval();
    u64 rand64();
    void zobrist_init();
    void zobrist_update();
    u64 zobrist_pieces[13][64];
    u64 zobrist_black;
    u64 zobrist_castling[16];
    u64 zobrist_enpassant[65];
//private:
    u64 positive_ray_attacks(u64 occupied, int direction, int square);
    u64 negative_ray_attacks(u64 occupied, int direction, int square);
    u64 general_ray_attacks(u64 occupied, int direction, int square);
    u64 diagonal_attacks(u64 occupied, int square);
    u64 antidiagonal_attacks(u64 occupied, int square);
    u64 rank_attacks(u64 occupied, int square);
    u64 file_attacks(u64 occupied, int square);
    u64 classical_rook_attacks(u64 occupied, int square);
    u64 classical_bishop_attacks(u64 occupied, int square);
    void slider_attacks_init();
    u64 rook_attacks(u64 occupied, int square);
    u64 bishop_attacks(u64 occupied, int square);
    u64 queen_attacks(u64 occupied, int square);
    bool square_attacked(u64 occupied, int square, bool side);
    u64 attacks_to_square(u64 occ, int square);
    bool check(u64 occupied);
    bool draw(int num_reps);
    u64 checkers(u64 occupied);
    u64 xray_rook_attacks(u64 occupied, u64 blockers, int from_square);
    u64 xray_bishop_attacks(u64 occupied, u64 blockers, int from_square);
    u64 hashkey() {return hash[ply];}
public:
    u64 pieces[13] {
        0x000000000000ff00,
        0x00ff000000000000,
        0x0000000000000042,
        0x4200000000000000,
        0x0000000000000024,
        0x2400000000000000,
        0x0000000000000081,
        0x8100000000000000,
        0x0000000000000008,
        0x0800000000000000,
        0x0000000000000010,
        0x1000000000000000,
        0xffff00000000ffff,
    };
    const u64& occupied{pieces[12]};
    int board[64] {
        6,  2,  4,  8, 10,  4,  2,  6,
        0,  0,  0,  0,  0,  0,  0,  0,
        12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 12, 12,
        1,  1,  1,  1,  1,  1,  1,  1,
        7,  3,  5,  9, 11,  5,  3,  7
    };
    bool side_to_move{true};
    int ply{};
    int enpassant_square[1024] {64};
    int castling_rights[1024] {15};
    int halfmove_clock[1024] {0};
    u64 hash[1024] {};
    int eval_stack[1024] {};
    Move move_stack[1024] {};
    int eval_phase();
    int mg_static_eval{};
    int eg_static_eval{};
    Position() {
        pst_init();
        slider_attacks_init();
        zobrist_init();
    }
};

template <bool side> inline u64 Position::pawns_forward_one(u64 pawns) {
    if constexpr (side) return pawns >> 8;
    else return pawns << 8;
}

template <bool side> inline u64 Position::pawns_backward_one(u64 pawns) {
    if constexpr (side) return pawns << 8;
    else return pawns >> 8;
}

template <bool side> inline u64 Position::pawns_forward_two(u64 pawns) {
    if constexpr (side) return pawns >> 16;
    else return pawns << 16;
}

template <bool side> inline u64 Position::pawns_backward_two(u64 pawns) {
    if constexpr (side) return pawns << 16;
    else return pawns >> 16;
}

template <bool side> inline u64 Position::pawns_forward_left(u64 pawns) {
    if constexpr (side) return (pawns & ~0x0101010101010101) >> 9;
    else return (pawns & ~0x0101010101010101) << 7;
}

template <bool side> inline u64 Position::pawns_backward_left(u64 pawns) {
    if constexpr (side) return (pawns & ~0x0101010101010101) << 7;
    else return (pawns & ~0x0101010101010101) >> 9;
}

template <bool side> inline u64 Position::pawns_forward_right(u64 pawns) {
    if constexpr (side) return (pawns & ~0x8080808080808080) >> 7;
    else return (pawns & ~0x8080808080808080) << 9;
}

template <bool side> inline u64 Position::pawns_backward_right(u64 pawns) {
    if constexpr (side) return (pawns & ~0x8080808080808080) << 9;
    else return (pawns & ~0x8080808080808080) >> 7;
}

template <bool side> inline u64 Position::promotion_rank() {
    if constexpr (side) return 0x000000000000FF00;
    else return 0x00FF000000000000;
}

template <bool side> inline u64 Position::second_rank() {
    if constexpr (side) return 0x00FF000000000000;
    else return 0x000000000000FF00;
}

inline void Position::make_null() {
    ++ply;
    hash[ply] = hash[ply - 1];
    hash[ply] ^= zobrist_enpassant[enpassant_square[ply - 1]];
    hash[ply] ^= zobrist_black;
    side_to_move = !side_to_move;
    castling_rights[ply] = castling_rights[ply - 1];
    enpassant_square[ply] = 64;
    hash[ply] ^= zobrist_enpassant[64];
}

inline void Position::undo_null() {
    side_to_move = !side_to_move;
    --ply;
}

std::ostream& operator<<(std::ostream& out, Position& position);

#endif
