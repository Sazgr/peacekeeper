#include "bit_operations.h"
#include "board.h"
#include <iostream>

u64 Position::positive_ray_attacks(u64 occupied, int direction, int square) {
    u64 attacks = rays[direction][square];
    square = get_lsb((attacks & occupied) | 0x8000000000000000ull);
    attacks ^= rays[direction][square];
    return attacks;
}

u64 Position::negative_ray_attacks(u64 occupied, int direction, int square) {
    u64 attacks = rays[direction][square];
    square = get_msb((attacks & occupied) | 0x0000000000000001ull);
    attacks ^= rays[direction][square];
    return attacks;
}

u64 Position::general_ray_attacks(u64 occupied, int direction, int square) {
    u64 attacks = rays[direction][square];
    square = bitscan((attacks & occupied) | (1ull << (63*(direction & 4))), !(direction & 4));
    attacks ^= rays[direction][square];
    return attacks;
}

u64 Position::diagonal_attacks(u64 occupied, int square) {
    return positive_ray_attacks(occupied, southeast, square) | negative_ray_attacks(occupied, northwest, square);
}

u64 Position::antidiagonal_attacks(u64 occupied, int square) {
    return positive_ray_attacks(occupied, southwest, square) | negative_ray_attacks(occupied, northeast, square);
}

u64 Position::rank_attacks(u64 occupied, int square) {
    return positive_ray_attacks(occupied, east, square) | negative_ray_attacks(occupied, west, square);
}

u64 Position::file_attacks(u64 occupied, int square) {
    return positive_ray_attacks(occupied, south, square) | negative_ray_attacks(occupied, north, square);
}

void Position::slider_attacks_init() {
    //bishops
    for (int square{}; square < 64; ++square) {
        u64 attack_mask = bishop_premask[square];
        int num_sets = (1 << popcount(attack_mask));
        u64 temp;
        for (int index{}; index < num_sets; ++index) {
            temp = attack_mask;
            u64 occupancy = 0;
            int squre{};
            for (int count = 0; count < popcount(attack_mask); count++){
                squre = pop_lsb(temp);
                if (index & (1 << count)){
                    occupancy |= (1ULL << squre);
        		}
            }
            u64 magic_index = (occupancy * bishop_magics[square].magic) >> 55;
            lookup_table[bishop_magics[square].start + magic_index] = classical_bishop_attacks(occupancy, square);
        }
    }
    //rooks
    for (int square{}; square < 64; ++square) {
        u64 attack_mask = rook_premask[square];
        int num_sets = (1 << popcount(attack_mask));
        u64 temp;
        for (int index{}; index < num_sets; ++index) {
            temp = attack_mask;
            u64 occupancy = 0;
            int squre{};
            for (int count = 0; count < popcount(attack_mask); count++){
                squre = pop_lsb(temp);
                if (index & (1 << count)){
                    occupancy |= (1ULL << squre);
        		}
            }
            u64 magic_index = (occupancy * rook_magics[square].magic >> 52);
            lookup_table[rook_magics[square].start + magic_index] = classical_rook_attacks(occupancy, square);
        }
    }
}

u64 Position::classical_rook_attacks(u64 occupied, int square) {
    return file_attacks(occupied, square) | rank_attacks(occupied, square);
}

u64 Position::classical_bishop_attacks(u64 occupied, int square) {
    return diagonal_attacks(occupied, square) | antidiagonal_attacks(occupied, square);
}

u64 Position::rook_attacks(u64 occupied, int square) {
    return lookup_table[rook_magics[square].start + ((occupied & rook_premask[square]) * rook_magics[square].magic >> 52)];
}

u64 Position::bishop_attacks(u64 occupied, int square) {
    return lookup_table[bishop_magics[square].start + ((occupied & bishop_premask[square]) * bishop_magics[square].magic >> 55)];
}

u64 Position::queen_attacks(u64 occupied, int square) {
    return rook_attacks(occupied, square) | bishop_attacks(occupied, square);
}


bool Position::square_attacked(u64 occupied, int square, bool side) {
    return (pawn_attacks[side ^ 1][square] & pieces[black_pawn + side]) 
    || (knight_attacks[square] & pieces[black_knight + side])
    || (king_attacks[square] & pieces[black_king + side])
    || (bishop_attacks(occupied, square) & (pieces[black_queen + side] | pieces[black_bishop + side]))
    || (rook_attacks(occupied, square) & (pieces[black_queen + side] | pieces[black_rook + side]));
}

bool Position::check(u64 occupied) {
    return square_attacked(occupied, get_lsb(pieces[black_king + side_to_move]), !side_to_move);
}

bool Position::check() {
    return square_attacked(occupied, get_lsb(pieces[black_king + side_to_move]), !side_to_move);
}

bool Position::draw() {
    if (halfmove_clock[ply] < 8) return false;
    if (halfmove_clock[ply] >= 100) return true;
    u64 curr_hash = hash[ply];
    int repeats{};
    for (int i{ply - 4}; i >= ply - halfmove_clock[ply] && repeats < 2; i -= 2) {
        repeats += (hash[i] == curr_hash);
    }
    return (repeats >= 2);
}

u64 Position::checkers(u64 occupied) {
    int square = get_lsb(pieces[side_to_move + 10]);
    u64 checkers{};
    u64 pawns = pieces[black_pawn + !side_to_move];
    checkers |= pawn_attacks[side_to_move][square] & pawns;
    u64 knights = pieces[black_knight + !side_to_move];
    checkers |= knight_attacks[square] & knights;
    u64 king = pieces[black_king + !side_to_move];
    checkers |= king_attacks[square] & king;
    u64 bishops_queens = pieces[black_queen + !side_to_move] | pieces[black_bishop + !side_to_move];
    checkers |= bishop_attacks(occupied, square) & bishops_queens;
    u64 rooks_queens = pieces[black_queen + !side_to_move] | pieces[black_rook + !side_to_move];
    checkers |= rook_attacks(occupied, square) & rooks_queens;
    return checkers;
}

u64 Position::xray_rook_attacks(u64 occupied, u64 blockers, int from_square) {
   u64 attacks = rook_attacks(occupied, from_square);
   blockers &= attacks;
   return attacks ^ rook_attacks(occupied ^ blockers, from_square);
}

u64 Position::xray_bishop_attacks(u64 occupied, u64 blockers, int from_square) {
   u64 attacks = bishop_attacks(occupied, from_square);
   blockers &= attacks;
   return attacks ^ bishop_attacks(occupied ^ blockers, from_square);
}

u64 Position::attack_map(u64 occupied, bool side_to_attack) {
    u64 curr_board;
    u64 attack_map{};
    curr_board = pieces[side_to_attack];
    while (curr_board) {
        attack_map |= pawn_attacks[side_to_attack][pop_lsb(curr_board)];
    }
    curr_board = pieces[side_to_attack + 2];
    while (curr_board) {
        attack_map |= knight_attacks[pop_lsb(curr_board)];
    }
    curr_board = pieces[side_to_attack + 4];
    while (curr_board) {
        attack_map |= bishop_attacks(occupied, pop_lsb(curr_board));
    }
    curr_board = pieces[side_to_attack + 6];
    while (curr_board) {
        attack_map |= rook_attacks(occupied, pop_lsb(curr_board));
    }
    curr_board = pieces[side_to_attack + 8];
    while (curr_board) {
        attack_map |= queen_attacks(occupied, pop_lsb(curr_board));
    }
    attack_map |= king_attacks[get_lsb(pieces[side_to_attack + 10])];
    return attack_map;
}

void Position::legal_moves(Movelist& movelist) {
    movelist.clear();
    int king_location = get_lsb(pieces[black_king + side_to_move]);
    u64 own_pieces{pieces[side_to_move] | pieces[side_to_move + 2] | pieces[side_to_move + 4] | pieces[side_to_move + 6] | pieces[side_to_move + 8] | pieces[side_to_move + 10]};
    u64 opp_pieces{occupied ^ own_pieces};
    u64 curr_board;
    u64 curr_moves;
    int piece_location;
    int ep_square = enpassant_square[ply];
    u64 ep_bb = 1ull << ep_square;
    u64 castling = castling_rights[ply] >> (2 * side_to_move);
    int shift = 56 * side_to_move;
    //generating pinned pieces
    u64 hv_pinmask{(xray_rook_attacks(occupied, own_pieces, king_location) & (pieces[black_rook + !side_to_move] | pieces[black_queen + !side_to_move]))};
    u64 dd_pinmask{(xray_bishop_attacks(occupied, own_pieces, king_location) & (pieces[black_bishop + !side_to_move] | pieces[black_queen + !side_to_move]))};
    
    curr_board = hv_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        hv_pinmask |= between[piece_location][king_location];
    }
    curr_board = dd_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        dd_pinmask |= between[piece_location][king_location];
    }
    u64 not_pinned{~(hv_pinmask | dd_pinmask)};
                     
    u64 curr_checkers{checkers(occupied)};
    u64 checkmask{0xFFFFFFFFFFFFFFFF};
    int end;
    u64 opp_attacks{attack_map(occupied^pieces[side_to_move + 10], !side_to_move)};
    curr_moves = king_attacks[king_location] & ~opp_attacks & ~own_pieces;
    while (curr_moves != 0) {
        end = pop_lsb(curr_moves);
        movelist.add(Move{board[king_location], king_location, board[end], end, none});
    }
    switch (popcount(curr_checkers)) {
        case 2:
            return; //double check
        case 1: //single check
            checkmask = between[get_lsb(curr_checkers)][king_location] ^ curr_checkers;
            //pinned pieces cannot move to evade check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & curr_checkers;
                curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & ~occupied;
                curr_moves &= checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & curr_checkers;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied;
                curr_moves &= checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & ~own_pieces & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~own_pieces & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//horizontal/vertical sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~own_pieces & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            if (ep_bb && curr_checkers == (1ull << (ep_square ^ 8))) {
                curr_board = pawn_attacks[!side_to_move][ep_square] & pieces[side_to_move] & ~hv_pinmask;//possible capturing pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    movelist.add(Move{board[piece_location], piece_location, 12, ep_square, enpassant});
                }
            }
            return;
        case 0: // no check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;//non-pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces;
                curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & (hv_pinmask | dd_pinmask);//pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces & dd_pinmask;
                curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & (~occupied) & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & not_pinned;//non-pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & (hv_pinmask | dd_pinmask);//pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces & dd_pinmask;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;//non-pinned knights, pinned ones cannot move
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & ~own_pieces;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//non-pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~own_pieces;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & dd_pinmask;//pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~own_pieces & dd_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//non-pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~own_pieces;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & hv_pinmask;//pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~own_pieces & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pawn_attacks[!side_to_move][ep_square] & pieces[side_to_move] & ~hv_pinmask & ((dd_pinmask & ep_bb)?0xFFFFFFFFFFFFFFFF:~dd_pinmask);//possible capturing pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if ((rank_attacks(occupied ^ (1ull << (ep_square ^ 8)) ^ (1ull << piece_location), king_location) & (pieces[!side_to_move + 6] | pieces[!side_to_move + 8])) == 0ull) {
                    movelist.add(Move{board[piece_location], piece_location, 12, ep_square, enpassant});
                }
            }
            if ((castling & 2) && (((occupied >> shift) & 0xF0ull) == 0x90ull) && !(opp_attacks & (0x70ull << shift))) { //kingside
                movelist.add(Move{black_king+side_to_move, shift+4, empty_square, shift+6, k_castling});
            }
            if ((castling & 1) && (((occupied >> shift) & 0x1Full) == 0x11ull) && !(opp_attacks & (0x1Cull << shift))) { //queenside
                movelist.add(Move{black_king+side_to_move, shift+4, empty_square, shift+2, q_castling});
            }
            return;
    }
}
    
void Position::print_bitboard(u64 bits) {
    for (int rank{0}; rank<8; ++rank) {
        for (int file{0}; file<8; ++file) {
            std::cout << ((bits & 1)?"#":".");
            std::cout << " ";
            bits >>= 1;
        }
        std::cout << "\n";
    }
}

void Position::legal_noisy(Movelist& movelist) {
    movelist.clear();
    int king_location = get_lsb(pieces[black_king + side_to_move]);
    u64 own_pieces{pieces[side_to_move] | pieces[side_to_move + 2] | pieces[side_to_move + 4] | pieces[side_to_move + 6] | pieces[side_to_move + 8] | pieces[side_to_move + 10]};
    u64 opp_pieces{occupied ^ own_pieces};
    u64 curr_board;
    u64 curr_moves;
    int piece_location;
    int ep_square = enpassant_square[ply];
    u64 ep_bb = 1ull << ep_square;
    //generating pinned pieces
    u64 hv_pinmask{(xray_rook_attacks(occupied, own_pieces, king_location) & (pieces[black_rook + !side_to_move] | pieces[black_queen + !side_to_move]))};
    u64 dd_pinmask{(xray_bishop_attacks(occupied, own_pieces, king_location) & (pieces[black_bishop + !side_to_move] | pieces[black_queen + !side_to_move]))};
    
    curr_board = hv_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        hv_pinmask |= between[piece_location][king_location];
    }
    curr_board = dd_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        dd_pinmask |= between[piece_location][king_location];
    }
    u64 not_pinned{~(hv_pinmask | dd_pinmask)};
                     
    u64 curr_checkers{checkers(occupied)};
    u64 checkmask{0xFFFFFFFFFFFFFFFF};
    int end;
    u64 opp_attacks{attack_map(occupied^pieces[side_to_move + 10], !side_to_move)};
    curr_moves = king_attacks[king_location] & ~opp_attacks & opp_pieces;
    while (curr_moves != 0) {
        end = pop_lsb(curr_moves);
        movelist.add(Move{board[king_location], king_location, board[end], end, none});
    }
    switch (popcount(curr_checkers)) {
        case 2:
            return; //double check
        case 1: //single check
            checkmask = between[get_lsb(curr_checkers)][king_location] ^ curr_checkers;
            //pinned pieces cannot move to evade check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & curr_checkers;
                curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & ~occupied;
                curr_moves &= checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & curr_checkers;
                curr_moves &= checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & opp_pieces & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & opp_pieces & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//horizontal/vertical sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & opp_pieces & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            if (ep_bb && curr_checkers == (1ull << (ep_square ^ 8))) {
                curr_board = pawn_attacks[!side_to_move][ep_square] & pieces[side_to_move] & ~hv_pinmask;//possible capturing pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    movelist.add(Move{board[piece_location], piece_location, 12, ep_square, enpassant});
                }
            }
            return;
        case 0: // no check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;//non-pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & (hv_pinmask | dd_pinmask);//pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces & dd_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & not_pinned;//non-pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & (hv_pinmask | dd_pinmask);//pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces & dd_pinmask;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;//non-pinned knights, pinned ones cannot move
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & opp_pieces;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//non-pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & opp_pieces;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & dd_pinmask;//pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & opp_pieces & dd_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//non-pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & opp_pieces;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & hv_pinmask;//pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & opp_pieces & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pawn_attacks[!side_to_move][ep_square] & pieces[side_to_move] & ~hv_pinmask & ((dd_pinmask & ep_bb)?0xFFFFFFFFFFFFFFFF:~dd_pinmask);//possible capturing pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if ((rank_attacks(occupied ^ (1ull << (ep_square ^ 8)) ^ (1ull << piece_location), king_location) & (pieces[!side_to_move + 6] | pieces[!side_to_move + 8])) == 0ull) {
                    movelist.add(Move{board[piece_location], piece_location, 12, ep_square, enpassant});
                }
            }
            return;
    }
}

void Position::legal_quiet(Movelist& movelist) {
    movelist.clear();
    int king_location = get_lsb(pieces[black_king + side_to_move]);
    u64 own_pieces{pieces[side_to_move] | pieces[side_to_move + 2] | pieces[side_to_move + 4] | pieces[side_to_move + 6] | pieces[side_to_move + 8] | pieces[side_to_move + 10]};
    u64 opp_pieces{occupied ^ own_pieces};
    u64 curr_board;
    u64 curr_moves;
    int piece_location;
    int ep_square = enpassant_square[ply];
    u64 ep_bb = 1ull << ep_square;
    u64 castling = castling_rights[ply] >> (2 * side_to_move);
    int shift = 56 * side_to_move;
    //generating pinned pieces
    u64 hv_pinmask{(xray_rook_attacks(occupied, own_pieces, king_location) & (pieces[black_rook + !side_to_move] | pieces[black_queen + !side_to_move]))};
    u64 dd_pinmask{(xray_bishop_attacks(occupied, own_pieces, king_location) & (pieces[black_bishop + !side_to_move] | pieces[black_queen + !side_to_move]))};
    
    curr_board = hv_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        hv_pinmask |= between[piece_location][king_location];
    }
    curr_board = dd_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        dd_pinmask |= between[piece_location][king_location];
    }
    u64 not_pinned{~(hv_pinmask | dd_pinmask)};
                     
    u64 curr_checkers{checkers(occupied)};
    u64 checkmask{0xFFFFFFFFFFFFFFFF};
    int end;
    u64 opp_attacks{attack_map(occupied^pieces[side_to_move + 10], !side_to_move)};
    curr_moves = king_attacks[king_location] & ~opp_attacks & ~occupied;
    while (curr_moves != 0) {
        end = pop_lsb(curr_moves);
        movelist.add(Move{board[king_location], king_location, board[end], end, none});
    }
    switch (popcount(curr_checkers)) {
        case 2:
            return; //double check
        case 1: //single check
            checkmask = between[get_lsb(curr_checkers)][king_location] ^ curr_checkers;
            //pinned pieces cannot move to evade check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & ~occupied;
                curr_moves &= checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & ~occupied & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~occupied & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//horizontal/vertical sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~occupied & checkmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            return;
        case 0: // no check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;//non-pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & (hv_pinmask | dd_pinmask);//pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & (~occupied) & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;//non-pinned knights, pinned ones cannot move
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//non-pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & dd_pinmask;//pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~occupied & dd_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//non-pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & hv_pinmask;//pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~occupied & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            if ((castling & 2) && (((occupied >> shift) & 0xF0ull) == 0x90ull) && !(opp_attacks & (0x70ull << shift))) { //kingside
                movelist.add(Move{black_king+side_to_move, shift+4, empty_square, shift+6, k_castling});
            }
            if ((castling & 1) && (((occupied >> shift) & 0x1Full) == 0x11ull) && !(opp_attacks & (0x1Cull << shift))) { //queenside
                movelist.add(Move{black_king+side_to_move, shift+4, empty_square, shift+2, q_castling});
            }
            return;
    }
}

void Position::print(std::ostream& out) {
    std::vector<std::string> pieces{"♟", "♙", "♞", "♘", "♝", "♗", "♜", "♖", "♛", "♕", "♚", "♔", ".", "."};
    out << "8 ";
    for (int square{0}; square < 64; ++square) {
        out << pieces[board[square]^1] << ' ';
        if ((square & 7) == 7)
            out << '\n' << (7 - (square >> 3)) << ' ';
    }
    out << "a b c d e f g h\n";
}

std::ostream& operator<<(std::ostream& out, Position& position) {
    position.print(out);
    return out;
}

u64 Position::count_legal_moves() {
    u64 count{};
    int king_location = get_lsb(pieces[black_king + side_to_move]);
    u64 own_pieces{pieces[side_to_move] | pieces[side_to_move + 2] | pieces[side_to_move + 4] | pieces[side_to_move + 6] | pieces[side_to_move + 8] | pieces[side_to_move + 10]};
    u64 opp_pieces{occupied ^ own_pieces};
    u64 curr_board;
    u64 curr_moves;
    int piece_location;
    int ep_square = enpassant_square[ply];
    u64 ep_bb = 1ull << ep_square;
    u64 castling = castling_rights[ply] >> (2 * side_to_move);
    int shift = 56 * side_to_move;
    //generating pinned pieces
    u64 hv_pinmask{(xray_rook_attacks(occupied, own_pieces, king_location) & (pieces[black_rook + !side_to_move] | pieces[black_queen + !side_to_move]))};
    u64 dd_pinmask{(xray_bishop_attacks(occupied, own_pieces, king_location) & (pieces[black_bishop + !side_to_move] | pieces[black_queen + !side_to_move]))};
    
    curr_board = hv_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        hv_pinmask |= between[piece_location][king_location];
    }
    curr_board = dd_pinmask;
    while (curr_board) {
        piece_location = pop_lsb(curr_board);
        dd_pinmask |= between[piece_location][king_location];
    }
    u64 not_pinned{~(hv_pinmask | dd_pinmask)};
                     
    u64 curr_checkers{checkers(occupied)};
    u64 checkmask{0xFFFFFFFFFFFFFFFF};
    int end;
    u64 opp_attacks{attack_map(occupied^pieces[side_to_move + 10], !side_to_move)};
    curr_moves = king_attacks[king_location] & ~opp_attacks & ~own_pieces;
    count += popcount(curr_moves);
    switch (popcount(curr_checkers)) {
        case 2:
            return count; //double check
        case 1: //single check
            checkmask = between[get_lsb(curr_checkers)][king_location] ^ curr_checkers;
            //pinned pieces cannot move to evade check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & curr_checkers;
                curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & ~occupied;
                curr_moves &= checkmask;
                count += popcount(curr_moves);
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & curr_checkers;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied;
                curr_moves &= checkmask;
                count += 4 * popcount(curr_moves);
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & ~own_pieces & checkmask;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~own_pieces & checkmask;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//horizontal/vertical sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~own_pieces & checkmask;
                count += popcount(curr_moves);
            }
            if (ep_bb && curr_checkers == (1ull << (ep_square ^ 8))) {
                curr_board = pawn_attacks[!side_to_move][ep_square] & pieces[side_to_move] & ~hv_pinmask;//possible capturing pawns
                count += popcount(curr_board);
            }
            return count;
        case 0: // no check
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & not_pinned;//non-pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces;
                curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & ~occupied;
                count += popcount(curr_moves);
            }
            curr_board = pieces[side_to_move] & (side_to_move?0xFFFFFFFFFFFF0000:0x0000FFFFFFFFFFFF) & (hv_pinmask | dd_pinmask);//pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces & dd_pinmask;
                curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side_to_move][piece_location] & (~occupied) & hv_pinmask;
                count += popcount(curr_moves);
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & not_pinned;//non-pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied;
                count += 4 * popcount(curr_moves);
            }
            curr_board = pieces[side_to_move] & (side_to_move?0x000000000000FF00:0x00FF000000000000) & (hv_pinmask | dd_pinmask);//pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = pawn_attacks[side_to_move][piece_location] & opp_pieces & dd_pinmask;
                curr_moves |= pawn_pushes[side_to_move][piece_location] & ~occupied & hv_pinmask;
                count += 4 * popcount(curr_moves);
            }
            curr_board = pieces[side_to_move + 2] & not_pinned;//non-pinned knights, pinned ones cannot move
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & ~own_pieces;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & not_pinned;//non-pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~own_pieces;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side_to_move + 4] | pieces[side_to_move + 8]) & dd_pinmask;//pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & ~own_pieces & dd_pinmask;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & not_pinned;//non-pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~own_pieces;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side_to_move + 6] | pieces[side_to_move + 8]) & hv_pinmask;//pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & ~own_pieces & hv_pinmask;
                count += popcount(curr_moves);
            }
            curr_board = pawn_attacks[!side_to_move][ep_square] & pieces[side_to_move] & ~hv_pinmask & ((dd_pinmask & ep_bb)?0xFFFFFFFFFFFFFFFF:~dd_pinmask);//possible capturing pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                count += (!(rank_attacks(occupied ^ (1ull << (ep_square ^ 8)) ^ (1ull << piece_location), king_location) & (pieces[!side_to_move + 6] | pieces[!side_to_move + 8])));
            }
            count += ((castling & 2) && (((occupied >> shift) & 0xF0ull) == 0x90ull) && !(opp_attacks & (0x70ull << shift))); //kingside
            count += ((castling & 1) && (((occupied >> shift) & 0x1Full) == 0x11ull) && !(opp_attacks & (0x1Cull << shift))); //queenside
            return count;
    }
    return count;
}

void Position::make_move(Move move) {
    ++ply;
    hash[ply] = hash[ply - 1];
    hash[ply] ^= zobrist_black;
    side_to_move = !side_to_move;
    int start = move.start();
    int end = move.end();
    int piece = move.piece();
    int captured = move.captured();
    fill_sq<true>(start, empty_square);
    switch (move.flag()) {
        case none:
            fill_sq<true>(end, piece);
            eval_phase = eval_phase - gamephase[captured];
            break;
        case knight_pr:
            fill_sq<true>(end, piece + 2);
            eval_phase = eval_phase - gamephase[captured] + 1;
            break;
        case bishop_pr:
            fill_sq<true>(end, piece + 4);
            eval_phase = eval_phase - gamephase[captured] + 1;
            break;
        case rook_pr:
            fill_sq<true>(end, piece + 6);
            eval_phase = eval_phase - gamephase[captured] + 2;
            break;
        case queen_pr:
            fill_sq<true>(end, piece + 8);
            eval_phase = eval_phase - gamephase[captured] + 4;
            break;
        case k_castling:
            fill_sq<true>(start + 3, empty_square);
            fill_sq<true>(end, piece);
            fill_sq<true>(start + 1, piece - 4);
            break;
        case q_castling:
            fill_sq<true>(start - 4, empty_square);
            fill_sq<true>(end, piece);
            fill_sq<true>(start - 1, piece - 4);
            break;
        case enpassant:
            fill_sq<true>(end ^ 8, empty_square);//ep square
            fill_sq<true>(end, piece);
            break;
    }
    enpassant_square[ply] = (!(piece & ~1) && end == (start ^ 16)) ? (end ^ 8) : 64;
    castling_rights[ply] = castling_rights[ply - 1] & castling_disable[start] & castling_disable[start];
    halfmove_clock[ply] = ((!(piece & ~1) || captured != 12) ? 0 : halfmove_clock[ply - 1] + 1);
    hash[ply] ^= zobrist_castling[castling_rights[ply - 1]] ^ zobrist_castling[castling_rights[ply]];
    hash[ply] ^= zobrist_enpassant[enpassant_square[ply - 1]] ^ zobrist_enpassant[enpassant_square[ply]];
}

void Position::undo_move(Move move) {
    side_to_move = !side_to_move;
    int start = move.start();
    int end = move.end();
    int piece = move.piece();
    int captured = move.captured();
    fill_sq<false>(start, piece);
    switch (move.flag()) {
        case none:
            fill_sq<false>(end, captured);
            eval_phase = eval_phase + gamephase[captured];
            break;
        case knight_pr:
            fill_sq<false>(end, captured);
            eval_phase = eval_phase + gamephase[captured] - 1;
            break;
        case bishop_pr:
            fill_sq<false>(end, captured);
            eval_phase = eval_phase + gamephase[captured] - 1;
            break;
        case rook_pr:
            fill_sq<false>(end, captured);
            eval_phase = eval_phase + gamephase[captured] - 2;
            break;
        case queen_pr:
            fill_sq<false>(end, captured);
            eval_phase = eval_phase + gamephase[captured] - 4;
            break;
        case k_castling:
            fill_sq<false>(end, empty_square);
            fill_sq<false>(start + 1, empty_square);
            fill_sq<false>(start + 3, piece - 4);
            break;
        case q_castling:
            fill_sq<false>(end, empty_square);
            fill_sq<false>(start - 1, empty_square);
            fill_sq<false>(start - 4, piece - 4);
            break;
        case enpassant:
            fill_sq<false>(end, empty_square);
            fill_sq<false>(end ^ 8, piece ^ 1);
            break;
    }
    --ply;
}

template <bool update_hash> inline void Position::fill_sq(int sq, int piece) {
    if constexpr (update_hash) hash[ply] ^= zobrist_pieces[board[sq]][sq] ^ zobrist_pieces[piece][sq];
    mg_static_eval = mg_static_eval - middlegame[board[sq]][sq] + middlegame[piece][sq];
    eg_static_eval = eg_static_eval - endgame[board[sq]][sq] + endgame[piece][sq];
    pieces[board[sq]] ^= (1ull << sq);
    pieces[piece] ^= (1ull << sq);
    board[sq] = piece;
}

int Position::static_eval() {
    int phase{std::min(eval_phase, 24)};
    return ((((2 * side_to_move - 1) * (mg_static_eval * phase + eg_static_eval * (24 - phase))) / 24) + eval_phase);
}

void Position::load(std::vector<int> position, bool stm) {
    ply = 0;
    for (int sq{0}; sq<position.size(); ++sq) {
        fill_sq<false>(sq, position[sq]);
    }
    side_to_move = stm;
    zobrist_update();
}

bool Position::load_fen(std::string fen_pos, std::string fen_stm, std::string fen_castling, std::string fen_ep, std::string fen_hmove_clock, std::string fen_fmove_counter) {
    int sq = 0;
    ply = 0;
    for (int i{}; i<64; ++i) {
        fill_sq<false>(i, empty_square);
    }
    for (auto pos = fen_pos.begin(); pos != fen_pos.end(); ++pos) {
        switch (*pos) {
            case 'p': fill_sq<false>(sq, black_pawn); break;
            case 'n': fill_sq<false>(sq, black_knight); break;
            case 'b': fill_sq<false>(sq, black_bishop); break;
            case 'r': fill_sq<false>(sq, black_rook); break;
            case 'q': fill_sq<false>(sq, black_queen); break;
            case 'k': fill_sq<false>(sq, black_king); break;
            case 'P': fill_sq<false>(sq, white_pawn); break;
            case 'N': fill_sq<false>(sq, white_knight); break;
            case 'B': fill_sq<false>(sq, white_bishop); break;
            case 'R': fill_sq<false>(sq, white_rook); break;
            case 'Q': fill_sq<false>(sq, white_queen); break;
            case 'K': fill_sq<false>(sq, white_king); break;
            case '/': --sq; break;
            case '1': break;
            case '2': ++sq; break;
            case '3': sq += 2; break;
            case '4': sq += 3; break;
            case '5': sq += 4; break;
            case '6': sq += 5; break;
            case '7': sq += 6; break;
            case '8': sq += 7; break;
            default: return false;
        }
        ++sq;
    }
    if (fen_stm == "w") {
        side_to_move = true;
    } else if (fen_stm == "b") {
        side_to_move = false;
    } else {
        return false;
    }
    castling_rights[0] = 0;
    for (auto pos = fen_castling.begin(); pos != fen_castling.end(); ++pos) {
        switch (*pos) {
            case '-': break;
            case 'q': castling_rights[0] |= 1; break;
            case 'k': castling_rights[0] |= 2; break;
            case 'Q': castling_rights[0] |= 4; break;
            case 'K': castling_rights[0] |= 8; break;
            default: return false;
        }
    }
    if (fen_ep == "-") {
        enpassant_square[0] = 64;
    } else if (fen_ep.size() == 2) {//ascii 'a' = 97 '8' = 56
        enpassant_square[0] = (static_cast<int>(fen_ep[0]) - 97) + 8 * (56 - static_cast<int>(fen_ep[1]));
    } else {
        return false;
    }
    halfmove_clock[0] = stoi(fen_hmove_clock);
    /******************************************************
     * FIX LATER: NOT PARSING FULLMOVE CLOCK *
     ******************************************************/
    zobrist_update();
    return true;
}

bool Position::parse_move(Move& out, std::string move) {
    if (move.size() < 4 || move.size() > 5) return false; 
    int start = (static_cast<int>(move[0]) - 97) + 8 * (56 - static_cast<int>(move[1]));//ascii 'a' = 97 , '8' = 56
    int end = (static_cast<int>(move[2]) - 97) + 8 * (56 - static_cast<int>(move[3]));
    if ((~63 & start) || (~63 & end)) return false; //out-of-bound squares
    int piece = board[start];
    int captured = board[end];
    int flag{none};
    if (move.size() == 5) {
        switch (move[4]) {
            case 'n': flag = knight_pr; break;
            case 'b': flag = bishop_pr; break;
            case 'r': flag = rook_pr; break;
            case 'q': flag = queen_pr; break;
        }
    } else {
        if (piece == black_king + side_to_move && end - start == 2) flag = k_castling;
        if (piece == black_king + side_to_move && end - start == -2) flag = q_castling;
        if (piece == black_pawn + side_to_move && (abs(end - start) == 7 || abs(end - start) == 9) && captured == empty_square) {
            flag = enpassant;
            captured = piece ^ 1;
        }
    }
    out = Move{piece, start, captured, end, flag};
    return true;
}

u64 Position::rand64() {
    static u64 next = 1;
    next = next * 1103515245 + 12345;
    return next;
}

void Position::zobrist_init() {
    for (int i{0}; i<12; ++i) {
        for (int j{0}; j<64; ++j) {
            zobrist_pieces[i][j] = rand64();
        }
    }
    for (int i{0}; i<64; ++i) {
        zobrist_pieces[12][i] = 0;
    }
    zobrist_black = rand64();
    for (int i{0}; i<64; ++i) {
        zobrist_enpassant[i] = rand64();
    }
    for (int i{0}; i<16; ++i) {
        zobrist_castling[i] = rand64();
    }
    zobrist_enpassant[64] = 0;
    zobrist_update();
}

void Position::zobrist_update() {
    hash[ply] = 0;
    for (int i{0}; i<64; ++i) {
        hash[ply] ^= zobrist_pieces[board[i]][i];
    }
    if (!side_to_move)
        hash[ply] ^= zobrist_black;
    hash[ply] ^= zobrist_castling[castling_rights[ply]];
    hash[ply] ^= zobrist_enpassant[enpassant_square[ply]];
}
