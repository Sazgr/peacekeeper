#include "bit_operations.h"
#include "board.h"
#include "nnue.h"
#include <cstring>
#include <iostream>

Position::Position() {
    pst_init();
    slider_attacks_init();
    nnue_init();
}

u64 Position::positive_ray_attacks(u64 occ, int direction, int square) {
    u64 attacks = rays[direction][square];
    square = get_lsb((attacks & occ) | 0x8000000000000000ull);
    attacks ^= rays[direction][square];
    return attacks;
}

u64 Position::negative_ray_attacks(u64 occ, int direction, int square) {
    u64 attacks = rays[direction][square];
    square = get_msb((attacks & occ) | 0x0000000000000001ull);
    attacks ^= rays[direction][square];
    return attacks;
}

u64 Position::general_ray_attacks(u64 occ, int direction, int square) {
    u64 attacks = rays[direction][square];
    square = bitscan((attacks & occ) | (1ull << (63*(direction & 4))), !(direction & 4));
    attacks ^= rays[direction][square];
    return attacks;
}

u64 Position::diagonal_attacks(u64 occ, int square) {
    return positive_ray_attacks(occ, southeast, square) | negative_ray_attacks(occ, northwest, square);
}

u64 Position::antidiagonal_attacks(u64 occ, int square) {
    return positive_ray_attacks(occ, southwest, square) | negative_ray_attacks(occ, northeast, square);
}

u64 Position::rank_attacks(u64 occ, int square) {
    return positive_ray_attacks(occ, east, square) | negative_ray_attacks(occ, west, square);
}

u64 Position::file_attacks(u64 occ, int square) {
    return positive_ray_attacks(occ, south, square) | negative_ray_attacks(occ, north, square);
}

void Position::slider_attacks_init() {
    for (int slider_loc{}; slider_loc < 64; ++slider_loc) { //bishops
        u64 attack_mask = bishop_premask[slider_loc];
        int num_sets = (1 << popcount(attack_mask));
        u64 temp;
        for (int index{}; index < num_sets; ++index) {
            temp = attack_mask;
            u64 occupancy = 0;
            int square{};
            for (int count = 0; count < popcount(attack_mask); count++) {
                square = pop_lsb(temp);
                if (index & (1 << count)) occupancy |= (1ull << square);
            }
            u64 magic_index = (occupancy * bishop_magics[slider_loc].magic) >> 55;
            lookup_table[bishop_magics[slider_loc].start + magic_index] = classical_bishop_attacks(occupancy, slider_loc);
        }
    }
    for (int slider_loc{}; slider_loc < 64; ++slider_loc) { //rooks
        u64 attack_mask = rook_premask[slider_loc];
        int num_sets = (1 << popcount(attack_mask));
        u64 temp;
        for (int index{}; index < num_sets; ++index) {
            temp = attack_mask;
            u64 occupancy = 0;
            int square{};
            for (int count = 0; count < popcount(attack_mask); count++) {
                square = pop_lsb(temp);
                if (index & (1 << count)) occupancy |= (1ull << square);
            }
            u64 magic_index = (occupancy * rook_magics[slider_loc].magic >> 52);
            lookup_table[rook_magics[slider_loc].start + magic_index] = classical_rook_attacks(occupancy, slider_loc);
        }
    }
}

u64 Position::classical_rook_attacks(u64 occ, int square) {
    return file_attacks(occ, square) | rank_attacks(occ, square);
}

u64 Position::classical_bishop_attacks(u64 occ, int square) {
    return diagonal_attacks(occ, square) | antidiagonal_attacks(occ, square);
}

u64 Position::rook_attacks(u64 occ, int square) {
    return lookup_table[rook_magics[square].start + ((occ & rook_premask[square]) * rook_magics[square].magic >> 52)];
}

u64 Position::bishop_attacks(u64 occ, int square) {
    return lookup_table[bishop_magics[square].start + ((occ & bishop_premask[square]) * bishop_magics[square].magic >> 55)];
}

u64 Position::queen_attacks(u64 occ, int square) {
    return rook_attacks(occ, square) | bishop_attacks(occ, square);
}

bool Position::square_attacked(u64 occ, int square, bool side) {
    return (pawn_attacks[!side][square] & pieces[black_pawn + side]) 
    || (knight_attacks[square] & pieces[black_knight + side])
    || (king_attacks[square] & pieces[black_king + side])
    || (bishop_attacks(occ, square) & (pieces[black_queen + side] | pieces[black_bishop + side]))
    || (rook_attacks(occ, square) & (pieces[black_queen + side] | pieces[black_rook + side]));
}

u64 Position::attacks_to_square(u64 occ, int square) {
    return (pawn_attacks[0][square] & pieces[white_pawn])
    | (pawn_attacks[1][square] & pieces[black_pawn]) 
    | (knight_attacks[square] & (pieces[black_knight] | pieces[white_knight]))
    | (king_attacks[square] & (pieces[black_king] | pieces[white_king]))
    | (bishop_attacks(occ, square) & (pieces[black_queen] | pieces[black_bishop] | pieces[white_queen] | pieces[white_bishop]))
    | (rook_attacks(occ, square) & (pieces[black_queen] | pieces[black_rook] | pieces[white_queen] | pieces[white_rook]));
}

bool Position::check(u64 occ) {
    return square_attacked(occ, get_lsb(pieces[black_king + side_to_move]), !side_to_move);
}

bool Position::check() {
    return square_attacked(occupied, get_lsb(pieces[black_king + side_to_move]), !side_to_move);
}

bool Position::draw(int num_reps = 2) {
    if (halfmove_clock[ply] < 8) return false;
    if (halfmove_clock[ply] >= 100) return true;
    u64 curr_hash = hash[ply];
    int repeats{};
    for (int i{ply - 4}; i >= ply - halfmove_clock[ply] && repeats < num_reps; i -= 2) repeats += (hash[i] == curr_hash);
    return (repeats >= num_reps);
}

u64 Position::checkers(u64 occ) {
    int square = get_lsb(pieces[side_to_move + 10]);
    return (pawn_attacks[side_to_move][square] & pieces[black_pawn + !side_to_move])
         | (knight_attacks[square] & pieces[black_knight + !side_to_move])
         | (king_attacks[square] & pieces[black_king + !side_to_move])
         | (bishop_attacks(occ, square) & (pieces[black_queen + !side_to_move] | pieces[black_bishop + !side_to_move]))
         | (rook_attacks(occ, square) & (pieces[black_queen + !side_to_move] | pieces[black_rook + !side_to_move]));
}

u64 Position::xray_rook_attacks(u64 occ, u64 blockers, int from_square) {
   u64 attacks = rook_attacks(occ, from_square);
   blockers &= attacks;
   return attacks ^ rook_attacks(occ ^ blockers, from_square);
}

u64 Position::xray_bishop_attacks(u64 occ, u64 blockers, int from_square) {
   u64 attacks = bishop_attacks(occ, from_square);
   blockers &= attacks;
   return attacks ^ bishop_attacks(occ ^ blockers, from_square);
}

template <bool side_to_attack> u64 Position::attack_map(u64 occ) {
    u64 curr_board;
    u64 attack_map{pawns_forward_left<side_to_attack>(pieces[side_to_attack]) | pawns_forward_right<side_to_attack>(pieces[side_to_attack])};
    curr_board = pieces[side_to_attack + 2];
    while (curr_board) attack_map |= knight_attacks[pop_lsb(curr_board)];
    curr_board = pieces[side_to_attack + 4];
    while (curr_board) attack_map |= bishop_attacks(occ, pop_lsb(curr_board));
    curr_board = pieces[side_to_attack + 6];
    while (curr_board) attack_map |= rook_attacks(occ, pop_lsb(curr_board));
    curr_board = pieces[side_to_attack + 8];
    while (curr_board) attack_map |= queen_attacks(occ, pop_lsb(curr_board));
    attack_map |= king_attacks[get_lsb(pieces[side_to_attack + 10])];
    return attack_map;
}

template <Move_types types, bool side> void Position::gen_legal(Movelist& movelist) {
    constexpr bool gen_quiet = types & 1;
    constexpr bool gen_noisy = types & 2;
    movelist.clear();
    u64 own_pieces{pieces[side] | pieces[side + 2] | pieces[side + 4] | pieces[side + 6] | pieces[side + 8] | pieces[side + 10]};
    u64 opp_pieces{occupied ^ own_pieces};
    u64 valid_targets{};
    if constexpr (gen_quiet) valid_targets |= ~occupied;
    if constexpr (gen_noisy) valid_targets |= opp_pieces;
    u64 curr_board;
    u64 curr_moves;
    int piece_location;
    u64 promote_mask{promotion_rank<side>()};
    u64 nonpromote_mask{~promote_mask};
    int king_location = get_lsb(pieces[black_king + side]);
    int ep_square = enpassant_square[ply];
    u64 ep_bb = 1ull << ep_square;
    bool king_castle, queen_castle;
    int shift;
    if constexpr (side) {
        king_castle = castling_rights[ply][3] != 64;
        queen_castle = castling_rights[ply][2] != 64;
        shift = 56;
    } else {
        king_castle = castling_rights[ply][1] != 64;
        queen_castle = castling_rights[ply][0] != 64;
        shift = 0;
    }
    //generating pinned pieces
    u64 hv_pinmask{(xray_rook_attacks(occupied, own_pieces, king_location) & (pieces[black_rook + !side] | pieces[black_queen + !side]))};
    u64 dd_pinmask{(xray_bishop_attacks(occupied, own_pieces, king_location) & (pieces[black_bishop + !side] | pieces[black_queen + !side]))};
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
    int end;
    u64 opp_attacks{attack_map<!side>(occupied ^ pieces[side + 10])};
    curr_moves = king_attacks[king_location] & ~opp_attacks & valid_targets;
    while (curr_moves != 0) {
        end = pop_lsb(curr_moves);
        movelist.add(Move{board[king_location], king_location, board[end], end, none});
    }
    switch (popcount(curr_checkers)) {
        case 2:
            return; //double check
        case 1: //single check
            valid_targets &= between[get_lsb(curr_checkers)][king_location] ^ curr_checkers;
            //Note: Pinned pieces cannot move to evade check
            curr_board = pieces[side] & nonpromote_mask & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & curr_checkers;
                if constexpr (gen_quiet) curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side][piece_location] & ~occupied;
                curr_moves &= valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side] & promote_mask & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & curr_checkers;
                if constexpr (gen_quiet) curr_moves |= pawn_pushes[side][piece_location] & ~occupied;
                curr_moves &= valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            curr_board = pieces[side + 2] & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side + 4] | pieces[side + 8]) & not_pinned;//diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side + 6] | pieces[side + 8]) & not_pinned;//horizontal/vertical sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            if constexpr (gen_noisy) {
                if (ep_bb && curr_checkers == (1ull << (ep_square ^ 8))) {
                    curr_board = pawn_attacks[!side][ep_square] & pieces[side] & ~hv_pinmask;//possible capturing pawns
                    while (curr_board != 0) {
                        piece_location = pop_lsb(curr_board);
                        movelist.add(Move{board[piece_location], piece_location, 12, ep_square, enpassant});
                    }
                }
            }
            return;
        case 0: // no check
            /*if constexpr (gen_noisy) {
                //left capture
                curr_board = pieces[side] & not_pinned & pawns_backward_right<side>(opp_pieces);
                curr_moves = curr_board & promote_mask;
                while (curr_moves != 0) {
                    piece_location = pop_lsb(curr_moves);
                    if constexpr (side) end = piece_location - 9;
                    else end = piece_location + 7;
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
                curr_moves = curr_board & ~promote_mask;
                while (curr_moves != 0) {
                    piece_location = pop_lsb(curr_moves);
                    if constexpr (side) end = piece_location - 9;
                    else end = piece_location + 7;
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
                //right capture
                curr_board = pieces[side] & not_pinned & pawns_backward_left<side>(opp_pieces);
                curr_moves = curr_board & promote_mask;
                while (curr_moves != 0) {
                    piece_location = pop_lsb(curr_moves);
                    if constexpr (side) end = piece_location - 9;
                    else end = piece_location + 7;
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
                curr_moves = curr_board & ~promote_mask;
                while (curr_moves != 0) {
                    piece_location = pop_lsb(curr_moves);
                    if constexpr (side) end = piece_location - 7;
                    else end = piece_location + 9;
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            if constexpr (gen_quiet) {
                curr_board = pieces[side] & not_pinned & pawns_backward_one<side>(~occupied);
                curr_moves = curr_board & promote_mask;
                while (curr_moves != 0) {
                    piece_location = pop_lsb(curr_moves);
                    if constexpr (side) end = piece_location - 8;
                    else end = piece_location + 8;
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
                curr_moves = curr_board & ~promote_mask;
                while (curr_moves != 0) {
                    piece_location = pop_lsb(curr_moves);
                    if constexpr (side) end = piece_location - 8;
                    else end = piece_location + 8;
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
                //double push
                curr_moves = pieces[side] & not_pinned & pawns_backward_one<side>(~occupied) & second_rank<side>() & pawns_backward_two<side>(~occupied);
                while (curr_moves != 0) {
                    piece_location = pop_lsb(curr_moves);
                    if constexpr (side) end = piece_location - 16;
                    else end = piece_location + 16;
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }*/
            curr_board = pieces[side] & nonpromote_mask & not_pinned;//non-pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & opp_pieces;
                if constexpr (gen_quiet) curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side][piece_location] & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = pieces[side] & promote_mask & not_pinned;//non-pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & opp_pieces;
                if constexpr (gen_quiet) curr_moves |= pawn_pushes[side][piece_location] & ~occupied;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                }
            }
            if constexpr (gen_quiet) {
                curr_board = pieces[side] & nonpromote_mask & hv_pinmask;//pinned non-promoting pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side][piece_location] & (~occupied) & hv_pinmask;
                    while (curr_moves != 0) {
                        end = pop_lsb(curr_moves);
                        movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                    }
                }
            }
            if constexpr (gen_noisy) {
                curr_board = pieces[side] & nonpromote_mask & dd_pinmask;//pinned non-promoting pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    curr_moves = pawn_attacks[side][piece_location] & opp_pieces & dd_pinmask;
                    while (curr_moves != 0) {
                        end = pop_lsb(curr_moves);
                        movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                    }
                }
            }
            curr_board = pieces[side] & promote_mask & dd_pinmask;//pinned promoting pawns
            //Note: HV-pinned pawns on 7th rank cannot push (or capture) in either case
            if constexpr (gen_noisy) {
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    curr_moves = pawn_attacks[side][piece_location] & opp_pieces & dd_pinmask;
                    while (curr_moves != 0) {
                        end = pop_lsb(curr_moves);
                        movelist.add(Move{board[piece_location], piece_location, board[end], end, knight_pr});
                        movelist.add(Move{board[piece_location], piece_location, board[end], end, bishop_pr});
                        movelist.add(Move{board[piece_location], piece_location, board[end], end, rook_pr});
                        movelist.add(Move{board[piece_location], piece_location, board[end], end, queen_pr});
                    }
                }
            }
            curr_board = pieces[side + 2] & not_pinned;//non-pinned knights
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            //Note: pinned knights cannot move
            curr_board = (pieces[side + 4] | pieces[side + 8]) & not_pinned;//non-pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side + 4] | pieces[side + 8]) & dd_pinmask;//pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & valid_targets & dd_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side + 6] | pieces[side + 8]) & not_pinned;//non-pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & valid_targets;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            curr_board = (pieces[side + 6] | pieces[side + 8]) & hv_pinmask;//pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & valid_targets & hv_pinmask;
                while (curr_moves != 0) {
                    end = pop_lsb(curr_moves);
                    movelist.add(Move{board[piece_location], piece_location, board[end], end, none});
                }
            }
            if constexpr (gen_noisy) {
                curr_board = pawn_attacks[!side][ep_square] & pieces[side] & ~hv_pinmask & ((dd_pinmask & ep_bb)?0xFFFFFFFFFFFFFFFF:~dd_pinmask);//possible capturing pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    if ((rank_attacks(occupied ^ (1ull << (ep_square ^ 8)) ^ (1ull << piece_location), king_location) & (pieces[!side + 6] | pieces[!side + 8])) == 0ull) {
                        movelist.add(Move{board[piece_location], piece_location, 12, ep_square, enpassant});
                    }
                }
            }
            if constexpr (gen_quiet) { //castling
                if (king_castle && !((occupied >> shift) & castling_empty[king_location - shift][castling_rights[ply][side * 2 + 1] - shift]) && !(opp_attacks & (castling_safe[king_location - shift][castling_rights[ply][side * 2 + 1] - shift] << shift))) { //kingside
                    movelist.add(Move{black_king + side, king_location, empty_square, castling_rights[ply][side * 2 + 1], k_castling});
                }
                if (queen_castle && !((occupied >> shift) & castling_empty[king_location - shift][castling_rights[ply][side * 2] - shift]) && !(opp_attacks & (castling_safe[king_location - shift][castling_rights[ply][side * 2] - shift] << shift))) { //queenside
                    movelist.add(Move{black_king + side, king_location, empty_square, castling_rights[ply][side * 2], q_castling});
                }
            }
            return;
    }
}

void Position::print_bitboard(u64 bits) {
    for (int rank{0}; rank < 8; ++rank) {
        for (int file{0}; file < 8; ++file) {
            std::cout << ((bits & 1)?"#":".") << " ";
            bits >>= 1;
        }
        std::cout << "\n";
    }
}

void Position::legal_moves(Movelist& movelist) {
    if (side_to_move) gen_legal<all, true>(movelist);
    else gen_legal<all, false>(movelist);
}

void Position::legal_noisy(Movelist& movelist) {
    if (side_to_move) gen_legal<noisy, true>(movelist);
    else gen_legal<noisy, false>(movelist);
}

void Position::legal_quiet(Movelist& movelist) {
    if (side_to_move) gen_legal<quiet, true>(movelist);
    else gen_legal<quiet, false>(movelist);
}

void Position::print(std::ostream& out) {
    std::vector<std::string> pieces{"p", "P", "n", "N", "b", "B", "r", "R", "q", "Q", "k", "K", ".", "."};
    out << "8 ";
    for (int square{0}; square < 64; ++square) {
        out << pieces[board[square]] << ' ';
        if ((square & 7) == 7) out << '\n' << (7 - (square >> 3)) << ' ';
    }
    out << "a b c d e f g h\n";
}

std::ostream& operator<<(std::ostream& out, Position& position) {
    position.print(out);
    return out;
}

template void Position::make_move<false>(Move move, NNUE* nnue);
template void Position::make_move<true>(Move move, NNUE* nnue);

template <bool update_nnue> void Position::make_move(Move move, NNUE* nnue) {
    if constexpr (update_nnue) nnue->push();
    ++ply;
    hash[ply] = hash[ply - 1];
    hash[ply] ^= zobrist_black;
    int start = move.start();
    int end = move.end();
    int piece = move.piece();
    int captured = move.captured();
    int king_end = end;
    fill_sq<update_nnue, true>(start, empty_square, nnue);
    switch (move.flag()) {
        case none:
            fill_sq<update_nnue, true>(end, piece, nnue);
            break;
        case knight_pr:
            fill_sq<update_nnue, true>(end, piece + 2, nnue);
            break;
        case bishop_pr:
            fill_sq<update_nnue, true>(end, piece + 4, nnue);
            break;
        case rook_pr:
            fill_sq<update_nnue, true>(end, piece + 6, nnue);
            break;
        case queen_pr:
            fill_sq<update_nnue, true>(end, piece + 8, nnue);
            break;
        case k_castling:
            fill_sq<update_nnue, true>(end, empty_square, nnue);
            fill_sq<update_nnue, true>((start & 56) + 6, piece, nnue);
            fill_sq<update_nnue, true>((start & 56) + 5, piece - 4, nnue);
            king_end = (start & 56) + 6;
            break;
        case q_castling:
            fill_sq<update_nnue, true>(end, empty_square, nnue);
            fill_sq<update_nnue, true>((start & 56) + 2, piece, nnue);
            fill_sq<update_nnue, true>((start & 56) + 3, piece - 4, nnue);
            king_end = (start & 56) + 2;
            break;
        case enpassant:
            fill_sq<update_nnue, true>(end ^ 8, empty_square, nnue);//ep square
            fill_sq<update_nnue, true>(end, piece, nnue);
            break;
    }
    enpassant_square[ply] = (!(piece & ~1) && end == (start ^ 16)) ? (end ^ 8) : 64;
    memcpy(castling_rights[ply], castling_rights[ply - 1], sizeof(int) * 4);
    if (castling_rights[ply][0] != 64 && board[castling_rights[ply][0]] != black_rook) castling_rights[ply][0] = 64;
    if (castling_rights[ply][1] != 64 && board[castling_rights[ply][1]] != black_rook) castling_rights[ply][1] = 64;
    if (castling_rights[ply][2] != 64 && board[castling_rights[ply][2]] != white_rook) castling_rights[ply][2] = 64;
    if (castling_rights[ply][3] != 64 && board[castling_rights[ply][3]] != white_rook) castling_rights[ply][3] = 64;
    if (piece == black_king) {
        castling_rights[ply][0] = 64;
        castling_rights[ply][1] = 64;
    }
    if (piece == white_king) {
        castling_rights[ply][2] = 64;
        castling_rights[ply][3] = 64;
    }
    halfmove_clock[ply] = ((!(piece & ~1) || captured != 12) ? 0 : halfmove_clock[ply - 1] + 1);
    for (int i{}; i<4; ++i) {
        hash[ply] ^= zobrist_castling[castling_rights[ply - 1][i]] ^ zobrist_castling[castling_rights[ply][i]];
    }
    hash[ply] ^= zobrist_enpassant[enpassant_square[ply - 1]] ^ zobrist_enpassant[enpassant_square[ply]];
    if constexpr (update_nnue) if (piece == black_king + side_to_move && (((start ^ king_end) & 4) || (buckets > 1 && king_buckets[start] != king_buckets[king_end]))) {
        nnue->refresh(*this);
        nnue_sub.clear();
        nnue_add.clear();
    }
    king_square[0] = get_lsb(pieces[10]);
    king_square[1] = get_lsb(pieces[11]);
    side_to_move = !side_to_move;
}

template void Position::undo_move<false>(Move move, NNUE* nnue);
template void Position::undo_move<true>(Move move, NNUE* nnue);

template <bool update_nnue> void Position::undo_move(Move move, NNUE* nnue) {
    if constexpr (update_nnue) nnue->pop();
    side_to_move = !side_to_move;
    int start = move.start();
    int end = move.end();
    int piece = move.piece();
    int captured = move.captured();
    switch (move.flag()) {
        case none:
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, captured, nnue);
            break;
        case knight_pr:
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, captured, nnue);
            break;
        case bishop_pr:
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, captured, nnue);
            break;
        case rook_pr:
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, captured, nnue);
            break;
        case queen_pr:
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, captured, nnue);
            break;
        case k_castling:
            fill_sq<false, false>((start & 56) + 6, empty_square, nnue);
            fill_sq<false, false>((start & 56) + 5, empty_square, nnue);
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, piece - 4, nnue);
            break;
        case q_castling:
            fill_sq<false, false>((start & 56) + 2, empty_square, nnue);
            fill_sq<false, false>((start & 56) + 3, empty_square, nnue);
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, piece - 4, nnue);
            break;
        case enpassant:
            fill_sq<false, false>(start, piece, nnue);
            fill_sq<false, false>(end, empty_square, nnue);
            fill_sq<false, false>(end ^ 8, piece ^ 1, nnue);
            break;
    }
    king_square[0] = get_lsb(pieces[10]);
    king_square[1] = get_lsb(pieces[11]);
    nnue_sub.clear();
    nnue_add.clear();
    --ply;
}

template <bool update_nnue, bool update_hash> inline void Position::fill_sq(int sq, int piece, NNUE* nnue) {
    if constexpr (update_hash) {
        hash[ply] ^= zobrist_pieces[board[sq]][sq] ^ zobrist_pieces[piece][sq];
    }
    if constexpr (update_nnue) {
        if (board[sq] != empty_square) nnue_sub.push_back({index(board[sq], sq, 0, king_square[0]), index(board[sq], sq, 1, king_square[1])});
        if (piece != empty_square) nnue_add.push_back({index(piece, sq, 0, king_square[0]), index(piece, sq, 1, king_square[1])});
    }
    pieces[board[sq]] ^= (1ull << sq);
    pieces[piece] ^= (1ull << sq);
    board[sq] = piece;
}

void Position::nnue_update_accumulator(NNUE& nnue) {
    while (nnue_sub.size() > nnue_add.size()) {
        nnue.update_accumulator_sub_sub_add(nnue_sub[0], nnue_sub[1], nnue_add[0]);
        nnue_sub.pop_back();
        nnue_sub.pop_back();
        nnue_add.pop_back();
    }
    while (!nnue_sub.empty()) {
        nnue.update_accumulator_sub_add(nnue_sub.back(), nnue_add.back());
        nnue_sub.pop_back();
        nnue_add.pop_back();
    }
}

int Position::eval_phase() {
    return popcount(pieces[2]) + popcount(pieces[3]) + popcount(pieces[4]) + popcount(pieces[5])
        + 2 * (popcount(pieces[6]) + popcount(pieces[7])) + 4 * (popcount(pieces[8]) + popcount(pieces[9]));
}

int Position::static_eval() {
    int phase{std::min(eval_phase(), 24)};
    u32 eval{};
    u64 black_pieces = pieces[0] | pieces[2] | pieces[4] | pieces[6] | pieces[8] | pieces[10];
    u64 white_pieces = pieces[1] | pieces[3] | pieces[5] | pieces[7] | pieces[9] | pieces[11];
    int bk = king_square[0], wk = king_square[1];
    for (u64 bb{pieces[0]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][0][sq] + full_king[1][wk][0][sq];
        bool is_doubled = (doubled[0][sq] & pieces[0]);
        if (!(passed[0][sq] & pieces[1]) && !is_doubled) {
            if (!(doubled[0][sq] & white_pieces)) {
                eval -= eval_passed_free[(sq >> 3) ^ 7];
            } else {
                eval -= eval_passed[(sq >> 3) ^ 7];
            }
        }
        if (is_doubled) {
            eval -= eval_doubled[(sq >> 3) ^ 7];
        }
        if (!(isolated[sq & 7] & pieces[0])) {
            eval -= eval_isolated[(sq >> 3) ^ 7];
        }
        if (pawn_attacks[1][sq] & pieces[0]) {
            eval -= eval_supported[(sq >> 3) ^ 7];
        }
        if ((sq & 7) != 7 && board[sq + 1] == 0) {
            eval -= eval_phalanx[(sq >> 3) ^ 7];
        }
    }
    for (u64 bb{pieces[1]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][1][sq] + full_king[1][wk][1][sq];
        bool is_doubled = (doubled[1][sq] & pieces[1]);
        if (!(passed[1][sq] & pieces[0]) && !is_doubled) {
            if (!(doubled[1][sq] & black_pieces)) {
                eval += eval_passed_free[sq >> 3];
            } else {
                eval += eval_passed[sq >> 3];
            }
        }
        if (is_doubled) {
            eval += eval_doubled[sq >> 3];
        }
        if (!(isolated[sq & 7] & pieces[1])) {
            eval += eval_isolated[sq >> 3];
        }
        if (pawn_attacks[0][sq] & pieces[1]) {
            eval += eval_supported[sq >> 3];
        }
        if ((sq & 7) != 7 && board[sq + 1] == 1) {
            eval += eval_phalanx[sq >> 3];
        }
    }
    for (u64 bb{pieces[2]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][2][sq] + full_king[1][wk][2][sq];
        u64 moves = knight_attacks[sq] & ~black_pieces;
        eval -= knight_mobility[popcount(moves)] + knight_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
    }
    for (u64 bb{pieces[3]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][3][sq] + full_king[1][wk][3][sq];
        u64 moves = knight_attacks[sq] & ~white_pieces;
        eval += knight_mobility[popcount(moves)] + knight_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
    }
    for (u64 bb{pieces[4]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][4][sq] + full_king[1][wk][4][sq];
        u64 moves = bishop_attacks(occupied & ~pieces[8], sq) & ~black_pieces;
        eval -= bishop_mobility[popcount(moves)] + bishop_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
    }
    for (u64 bb{pieces[5]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][5][sq] + full_king[1][wk][5][sq];
        u64 moves = bishop_attacks(occupied & ~pieces[9], sq) & ~white_pieces;
        eval += bishop_mobility[popcount(moves)] + bishop_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
    }
    //bishop pair eval
    if (popcount(pieces[4]) >= 2) eval -= bishop_pair;
    if (popcount(pieces[5]) >= 2) eval += bishop_pair;
    int file_status[8] = {3, 3, 3, 3, 3, 3, 3, 3}; // 0=open, 1=black pawn, 2=white pawn, 3=closed
    for (int i{}; i<7; ++i) {
        if (!(between[i][56 + i] & pieces[0])) file_status[i] &= ~1;
        if (!(between[i][56 + i] & pieces[1])) file_status[i] &= ~2;
    }
    for (u64 bb{pieces[6]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][6][sq] + full_king[1][wk][6][sq];
        u64 moves = rook_attacks(occupied & ~pieces[8], sq) & ~black_pieces;
        eval -= rook_mobility[popcount(moves)] + rook_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
        if (file_status[sq & 7] == 0) eval -= rook_open[sq & 7];
        if (file_status[sq & 7] == 2) eval -= rook_semiopen[sq & 7];
    }
    for (u64 bb{pieces[7]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][7][sq] + full_king[1][wk][7][sq];
        u64 moves = rook_attacks(occupied & ~pieces[9], sq) & ~white_pieces;
        eval += rook_mobility[popcount(moves)] + rook_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
        if (file_status[sq & 7] == 0) eval += rook_open[sq & 7];
        if (file_status[sq & 7] == 1) eval += rook_semiopen[sq & 7];
    }
    for (u64 bb{pieces[8]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][8][sq] + full_king[1][wk][8][sq];
        u64 moves = queen_attacks(occupied, sq) & ~black_pieces;
        eval -= queen_mobility[popcount(moves)] + queen_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
        if (file_status[sq & 7] == 0) eval -= queen_open[sq & 7];
        if (file_status[sq & 7] == 2) eval -= queen_semiopen[sq & 7];
    }
    for (u64 bb{pieces[9]}; bb;) {
        const int sq = pop_lsb(bb);
        eval += full_king[0][bk][9][sq] + full_king[1][wk][9][sq];
        u64 moves = queen_attacks(occupied, sq) & ~white_pieces;
        eval += queen_mobility[popcount(moves)] + queen_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
        if (file_status[sq & 7] == 0) eval += queen_open[sq & 7];
        if (file_status[sq & 7] == 1) eval += queen_semiopen[sq & 7];
    }
    if (file_status[bk & 7] == 0) eval -= king_open[bk & 7];
    if (file_status[bk & 7] == 2) eval -= king_semiopen[bk & 7];
    if (file_status[wk & 7] == 0) eval += king_open[wk & 7];
    if (file_status[wk & 7] == 1) eval += king_semiopen[wk & 7];
    return ((2 * side_to_move - 1) * (static_cast<i16>(eval >> 16) * phase + static_cast<i16>(eval & 0xFFFF) * (24 - phase)) / 24) + phase / 2;
}

int Position::static_eval(NNUE& nnue) {
    nnue_update_accumulator(nnue);
#ifdef DATAGEN
    return nnue.evaluate(side_to_move);
#else
    return nnue.evaluate(side_to_move) * (72 + eval_phase()) / 96;
#endif
}

std::string Position::export_fen() {
    static std::vector<std::string> pieces{"p", "P", "n", "N", "b", "B", "r", "R", "q", "Q", "k", "K", ".", "."};
    std::string fen{};
    int empty_streak = 0;
    for (int rank{}; rank < 8; ++rank) {
        for (int file{}; file < 8; ++file) {
            if (board[rank * 8 + file] == 12) {
                ++empty_streak;
            } else {
                if (empty_streak) fen += std::to_string(empty_streak);
                empty_streak = 0;
                fen += pieces[board[rank * 8 + file]];
            }
        }
        if (empty_streak) {
            fen += std::to_string(empty_streak);
            empty_streak = 0;
        }
        if (rank != 7) fen += "/";
    }
    if (side_to_move) fen += " w ";
    else fen += " b ";
    if (castling_rights[ply][3] != 64) fen += chess960 ? static_cast<char>((castling_rights[ply][3] - 56) + 65) : 'K';
    if (castling_rights[ply][2] != 64) fen += chess960 ? static_cast<char>((castling_rights[ply][2] - 56) + 65) : 'Q';
    if (castling_rights[ply][1] != 64) fen += chess960 ? static_cast<char>(castling_rights[ply][1] + 97) : 'k';
    if (castling_rights[ply][0] != 64) fen += chess960 ? static_cast<char>(castling_rights[ply][0] + 97) : 'q';
    if (castling_rights[ply][0] == 64 && castling_rights[ply][1] == 64 && castling_rights[ply][2] == 64 && castling_rights[ply][3] == 64) fen += "-";
    if (enpassant_square[ply] == 64) fen += " -";
    else fen += " " + square_names[enpassant_square[ply]];
    fen += " " + std::to_string(halfmove_clock[ply]) + " " + std::to_string(ply / 2 + 1);
    return fen;
}

bool Position::load_fen(std::string fen_pos, std::string fen_stm, std::string fen_castling, std::string fen_ep, std::string fen_hmove_clock, std::string fen_fmove_counter) {
    int sq = 0;
    ply = 0;
    for (int i{}; i<64; ++i) fill_sq<false, false>(i, empty_square);
    for (auto pos = fen_pos.begin(); pos != fen_pos.end(); ++pos) {
        switch (*pos) {
            case 'p': fill_sq<false, false>(sq, black_pawn); break;
            case 'n': fill_sq<false, false>(sq, black_knight); break;
            case 'b': fill_sq<false, false>(sq, black_bishop); break;
            case 'r': fill_sq<false, false>(sq, black_rook); break;
            case 'q': fill_sq<false, false>(sq, black_queen); break;
            case 'k': fill_sq<false, false>(sq, black_king); break;
            case 'P': fill_sq<false, false>(sq, white_pawn); break;
            case 'N': fill_sq<false, false>(sq, white_knight); break;
            case 'B': fill_sq<false, false>(sq, white_bishop); break;
            case 'R': fill_sq<false, false>(sq, white_rook); break;
            case 'Q': fill_sq<false, false>(sq, white_queen); break;
            case 'K': fill_sq<false, false>(sq, white_king); break;
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
    king_square[0] = get_lsb(pieces[10]);
    king_square[1] = get_lsb(pieces[11]);
    if (fen_stm == "w") side_to_move = true;
    else if (fen_stm == "b") side_to_move = false;
    else return false;
    castling_rights[0][0] = castling_rights[0][1] = castling_rights[0][2] = castling_rights[0][3] = 64;
    if (chess960) {
        for (auto pos = fen_castling.begin(); pos != fen_castling.end(); ++pos) {
            switch (*pos) {
                case '-': break;
                case 'q': castling_rights[0][0] = get_lsb(pieces[6] & rays[west][king_square[0]]); break;
                case 'k': castling_rights[0][1] = get_lsb(pieces[6] & rays[east][king_square[0]]); break;
                case 'Q': castling_rights[0][2] = get_lsb(pieces[7] & rays[west][king_square[1]]); break;
                case 'K': castling_rights[0][3] = get_lsb(pieces[7] & rays[east][king_square[1]]); break;
                default:
                    if ((*pos) >= 97) { //lowercase means black castling rights
                        int king_location = king_square[0];
                        int rook_location = (static_cast<int>(*pos) - 97);
                        if (rook_location > king_location) {
                            castling_rights[0][1] = rook_location;
                        } else {
                            castling_rights[0][0] = rook_location;
                        }
                    } else { //upper case means white castling rights
                        int king_location = king_square[1];
                        int rook_location = (static_cast<int>(*pos) - 65) + 56;
                        if (rook_location > king_location) {
                            castling_rights[0][3] = rook_location;
                        } else {
                            castling_rights[0][2] = rook_location;
                        }
                    }
            }
        }
    } else {
        for (auto pos = fen_castling.begin(); pos != fen_castling.end(); ++pos) {
            switch (*pos) {
                case '-': break;
                case 'q': castling_rights[0][0] = 0; break;
                case 'k': castling_rights[0][1] = 7; break;
                case 'Q': castling_rights[0][2] = 56; break;
                case 'K': castling_rights[0][3] = 63; break;
                default: return false;
            }
        }
    }
    if (fen_ep == "-") enpassant_square[0] = 64;
    else if (fen_ep.size() == 2) enpassant_square[0] = (static_cast<int>(fen_ep[0]) - 97) + 8 * (56 - static_cast<int>(fen_ep[1]));//ascii 'a' = 97 '8' = 56
    else return false;
    halfmove_clock[0] = stoi(fen_hmove_clock);
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
        if (!chess960 && piece == black_king + side_to_move && end - start == 2) {
            flag = k_castling;
            end = start + 3;
            captured = empty_square;
        }
        if (!chess960 && piece == black_king + side_to_move && end - start == -2) {
            flag = q_castling;
            end = start - 4;
            captured = empty_square;
        }
        if (chess960 && piece == black_king + side_to_move && captured == black_rook + side_to_move && end > start) {
            flag = k_castling;
            captured = empty_square;
        }
        if (chess960 && piece == black_king + side_to_move && captured == black_rook + side_to_move && end < start) {
            flag = q_castling;
            captured = empty_square;
        }
        if (piece == black_pawn + side_to_move && (abs(end - start) == 7 || abs(end - start) == 9) && captured == empty_square) {
            flag = enpassant;
            captured = piece ^ 1;
        }
    }
    out = Move{piece, start, captured, end, flag};
    return true;
}

void Position::zobrist_update() {
    hash[ply] = 0;
    for (int i{0}; i < 64; ++i) hash[ply] ^= zobrist_pieces[board[i]][i];
    if (!side_to_move) hash[ply] ^= zobrist_black;
    for (int i{}; i < 4; ++i) {
        hash[ply] ^= zobrist_castling[castling_rights[ply][i]];
    }
    hash[ply] ^= zobrist_enpassant[enpassant_square[ply]];
}
