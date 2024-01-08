#ifndef PEACEKEEPER_MOVE
#define PEACEKEEPER_MOVE

#include "eval.h"
#include "options.h"
#include "typedefs.h"
#include <iostream>
#include <string>

const std::string square_names[66] {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "nu", "ll"//hack for printing null moves
};
enum Flags {
    none,
    knight_pr,
    bishop_pr,
    rook_pr,
    queen_pr,
    q_castling,
    k_castling,
    enpassant,
};
//24 bits
//PPPPSSSSSSCCCCEEEEEEEFFF
//321098765432109876543210
//842184218421842184218421

struct Move {
    Move() {
        data = (12 << 20) ^ (0 << 14) ^ (12 << 10) ^ (64 << 3) ^ none;
    }
    Move(int piece, int start_square, int captured_piece, int end_square, int flag = none) {
        data = (piece << 20) ^ (start_square << 14) ^ (captured_piece << 10) ^ (end_square << 3) ^ flag;
    }
    Move(const Move& rhs) {
        data = rhs.data;
    }
    Move (const u64 move_data) {
        data = move_data;
    }
    inline constexpr int flag() const {return data & 0x7;}
    inline constexpr int piece() const {return (data >> 20) & 0xF;}
    inline constexpr int start() const {return (data >> 14) & 0x3F;}
    inline constexpr int captured() const {return (data >> 10) & 0xF;}
    inline constexpr int end() const {return (data >> 3) & 0x7F;}
    inline constexpr bool is_null() const {return (data & 0x200);}
    inline constexpr bool not_null() const {return !(data & 0x200);}
    inline constexpr is_quiet() const {return captured() == 12 && flag() != queen_pr;}
    inline void add_sortkey(int key) {data = (data & 0xFFFFFFFF) | (static_cast<u64>(key) << 32);}
    inline int gain() const {
        return mg_value[captured() >> 1] + (flag() == queen_pr ? 939 : 0);
    }
    inline constexpr int sortkey() const{return data >> 32;}
    inline constexpr int evade_order() const {
        return 0;
    }
    inline int mvv_lva() const {
        if (flag() == queen_pr) return 384;
        if (flag() != none && !(flag() & 4)) return 0;
        return (captured() << 5) + piece() ^ 15;
    }
    void long_print(std::ostream& out = std::cout) {
        static const char piece_names[14]{'p', 'P', 'n', 'N', 'b', 'B', 'r', 'R', 'q', 'Q', 'k', 'K', '.', '.'};
        out << piece_names[piece()] << square_names[start()] << square_names[end()] << 'x' << piece_names[captured()];
        if (flag() == knight_pr) out << "=>n";
        if (flag() == bishop_pr) out << "=>b";
        if (flag() == rook_pr) out << "=>r";
        if (flag() == queen_pr) out << "=>q";
        if (flag() == enpassant) out << "=ep";
        out << '\n';
    }
    u64 data;
};

inline bool operator==(const Move& move1, const Move& move2) {
    return (move1.data & 0xFFFFFF) == (move2.data & 0xFFFFFF);
}

inline bool operator!=(const Move& move1, const Move& move2) {
    return (move1.data & 0xFFFFFF) != (move2.data & 0xFFFFFF);
}

inline bool operator<(const Move& move1, const Move& move2) {
    return move1.data > move2.data;//reversed so higher sortkeys go first by default
}

inline std::ostream& operator<<(std::ostream& out, const Move move) {
    if (!chess960 && move.flag() == q_castling) out << square_names[move.start()] << square_names[move.end() + 2];
    else if (!chess960 && move.flag() == k_castling) out << square_names[move.start()] << square_names[move.end() - 1];
    else out << square_names[move.start()] << square_names[move.end()];
    if (move.flag() == knight_pr) out << "n";
    if (move.flag() == bishop_pr) out << "b";
    if (move.flag() == rook_pr) out << "r";
    if (move.flag() == queen_pr) out << "q";
    return out;
}

#endif
