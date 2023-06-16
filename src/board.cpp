#include "bit_operations.h"
#include "board.h"
#include <iostream>

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
        king_castle = castling_rights[ply] & 8;
        queen_castle = castling_rights[ply] & 4;
        shift = 56;
    } else {
        king_castle = castling_rights[ply] & 2;
        queen_castle = castling_rights[ply] & 1;
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
            if constexpr (gen_quiet) {
                if (king_castle && (((occupied >> shift) & 0xF0ull) == 0x90ull) && !(opp_attacks & (0x70ull << shift))) { //kingside
                    movelist.add(Move{black_king+side, shift+4, empty_square, shift+6, k_castling});
                }
                if (queen_castle && (((occupied >> shift) & 0x1Full) == 0x11ull) && !(opp_attacks & (0x1Cull << shift))) { //queenside
                    movelist.add(Move{black_king+side, shift+4, empty_square, shift+2, q_castling});
                }
            }
            return;
    }
}

template <Move_types types, bool side> u64 Position::count_legal() {
    constexpr bool gen_quiet = types & 1;
    constexpr bool gen_noisy = types & 2;
    u64 count{};
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
        king_castle = castling_rights[ply] & 8;
        queen_castle = castling_rights[ply] & 4;
        shift = 56;
    } else {
        king_castle = castling_rights[ply] & 2;
        queen_castle = castling_rights[ply] & 1;
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
    count += popcount(curr_moves);
    switch (popcount(curr_checkers)) {
        case 2:
            return count; //double check
        case 1: //single check
            valid_targets &= between[get_lsb(curr_checkers)][king_location] ^ curr_checkers;
            //Note: Pinned pieces cannot move to evade check
            if constexpr (gen_noisy) {
                curr_board = pieces[side] & not_pinned & pawns_backward_right<side>(curr_checkers); //left capture
                count += 4 * popcount(curr_board & promote_mask) + popcount(curr_board & ~promote_mask);
                curr_board = pieces[side] & not_pinned & pawns_backward_left<side>(curr_checkers); //right capture
                count += 4 * popcount(curr_board & promote_mask) + popcount(curr_board & ~promote_mask);
            }
            if constexpr (gen_quiet) {
                curr_board = pieces[side] & not_pinned & pawns_backward_one<side>(~occupied); //push
                curr_moves = curr_board & pawns_backward_one<side>(valid_targets);
                count += 4 * popcount(curr_moves & promote_mask) + popcount(curr_moves & ~promote_mask);
                curr_moves = curr_board & second_rank<side>() & pawns_backward_two<side>(~occupied & valid_targets); //double push
                count += popcount(curr_moves);
            }
            /*curr_board = pieces[side] & nonpromote_mask & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & curr_checkers;
                if constexpr (gen_quiet) curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side][piece_location] & ~occupied;
                curr_moves &= valid_targets;
                count += popcount(curr_moves);
            }
            curr_board = pieces[side] & promote_mask & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & curr_checkers;
                if constexpr (gen_quiet) curr_moves |= pawn_pushes[side][piece_location] & ~occupied;
                curr_moves &= valid_targets;
                count += 4 * popcount(curr_moves);
            }*/
            curr_board = pieces[side + 2] & not_pinned;
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & valid_targets;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side + 4] | pieces[side + 8]) & not_pinned;//diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & valid_targets;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side + 6] | pieces[side + 8]) & not_pinned;//horizontal/vertical sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & valid_targets;
                count += popcount(curr_moves);
            }
            if constexpr (gen_noisy) {
                if (ep_bb && curr_checkers == (1ull << (ep_square ^ 8))) {
                    curr_board = pawn_attacks[!side][ep_square] & pieces[side] & ~hv_pinmask;//possible capturing pawns
                    count += popcount(curr_moves);
                }
            }
            return count;
        case 0: // no check
            if constexpr (gen_noisy) {
                curr_board = pieces[side] & not_pinned & pawns_backward_right<side>(opp_pieces); //left capture
                count += 4 * popcount(curr_board & promote_mask) + popcount(curr_board & ~promote_mask);
                curr_board = pieces[side] & not_pinned & pawns_backward_left<side>(opp_pieces); //right capture
                count += 4 * popcount(curr_board & promote_mask) + popcount(curr_board & ~promote_mask);
            }
            if constexpr (gen_quiet) {
                curr_board = pieces[side] & not_pinned & pawns_backward_one<side>(~occupied); //push
                count += 4 * popcount(curr_board & promote_mask) + popcount(curr_board & ~promote_mask);
                curr_moves = curr_board & second_rank<side>() & pawns_backward_two<side>(~occupied); //double push
                count += popcount(curr_moves);
            }
            /*curr_board = pieces[side] & nonpromote_mask & not_pinned;//non-pinned non-promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & opp_pieces;
                if constexpr (gen_quiet) curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side][piece_location] & ~occupied;
                count += popcount(curr_moves);
            }
            curr_board = pieces[side] & promote_mask & not_pinned;//non-pinned promoting pawns
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                if constexpr (gen_noisy) curr_moves = pawn_attacks[side][piece_location] & opp_pieces;
                if constexpr (gen_quiet) curr_moves |= pawn_pushes[side][piece_location] & ~occupied;
                count += 4 * popcount(curr_moves);
            }*/
            if constexpr (gen_noisy) {
                curr_board = pieces[side] & nonpromote_mask & (hv_pinmask | dd_pinmask);//pinned non-promoting pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    curr_moves = pawn_attacks[side][piece_location] & opp_pieces & dd_pinmask;
                    count += popcount(curr_moves);
                }
            }
            if constexpr (gen_quiet) { 
                curr_board = pieces[side] & nonpromote_mask & (hv_pinmask | dd_pinmask);//pinned non-promoting pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    curr_moves |= file_attacks(occupied, piece_location) & pawn_pushes[side][piece_location] & (~occupied) & hv_pinmask;
                    count += popcount(curr_moves);
                }
            }
            curr_board = pieces[side] & promote_mask & dd_pinmask;//pinned promoting pawns
            //Note: HV-pinned pawns on 7th rank cannot push (or capture) in either case
            if constexpr (gen_noisy) {
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    curr_moves = pawn_attacks[side][piece_location] & opp_pieces & dd_pinmask;
                    count += 4 * popcount(curr_moves);
                }
            }
            curr_board = pieces[side + 2] & not_pinned;//non-pinned knights
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = knight_attacks[piece_location] & valid_targets;
                count += popcount(curr_moves);
            }
            //Note: pinned knights cannot move
            curr_board = (pieces[side + 4] | pieces[side + 8]) & not_pinned;//non-pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & valid_targets;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side + 4] | pieces[side + 8]) & dd_pinmask;//pinned diagonal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = bishop_attacks(occupied, piece_location) & valid_targets & dd_pinmask;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side + 6] | pieces[side + 8]) & not_pinned;//non-pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & valid_targets;
                count += popcount(curr_moves);
            }
            curr_board = (pieces[side + 6] | pieces[side + 8]) & hv_pinmask;//pinned horizontal sliders
            while (curr_board != 0) {
                piece_location = pop_lsb(curr_board);
                curr_moves = rook_attacks(occupied, piece_location) & valid_targets & hv_pinmask;
                count += popcount(curr_moves);
            }
            if constexpr (gen_noisy) {
                curr_board = pawn_attacks[!side][ep_square] & pieces[side] & ~hv_pinmask & ((dd_pinmask & ep_bb)?0xFFFFFFFFFFFFFFFF:~dd_pinmask);//possible capturing pawns
                while (curr_board != 0) {
                    piece_location = pop_lsb(curr_board);
                    count += ((rank_attacks(occupied ^ (1ull << (ep_square ^ 8)) ^ (1ull << piece_location), king_location) & (pieces[!side + 6] | pieces[!side + 8])) == 0ull);
                }
            }
            if constexpr (gen_quiet) {
                count += (king_castle && (((occupied >> shift) & 0xF0ull) == 0x90ull) && !(opp_attacks & (0x70ull << shift))); //kingside
                count += (queen_castle && (((occupied >> shift) & 0x1Full) == 0x11ull) && !(opp_attacks & (0x1Cull << shift))); //queenside
            }
            return count;
    }
    return count;
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

u64 Position::count_legal_moves() {
    if (side_to_move) return count_legal<all, true>();
    else return count_legal<all, false>();
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

void Position::make_move(Move move) {
    ++ply;
    eval_stack[ply] = -20001;
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
            break;
        case knight_pr:
            fill_sq<true>(end, piece + 2);
            break;
        case bishop_pr:
            fill_sq<true>(end, piece + 4);
            break;
        case rook_pr:
            fill_sq<true>(end, piece + 6);
            break;
        case queen_pr:
            fill_sq<true>(end, piece + 8);
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
    castling_rights[ply] = castling_rights[ply - 1] & castling_disable[start] & castling_disable[end];
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
            break;
        case knight_pr:
            fill_sq<false>(end, captured);
            break;
        case bishop_pr:
            fill_sq<false>(end, captured);
            break;
        case rook_pr:
            fill_sq<false>(end, captured);
            break;
        case queen_pr:
            fill_sq<false>(end, captured);
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
    //mg_static_eval = mg_static_eval - middlegame[board[sq]][sq] + middlegame[piece][sq];
    //eg_static_eval = eg_static_eval - endgame[board[sq]][sq] + endgame[piece][sq];
    pieces[board[sq]] ^= (1ull << sq);
    pieces[piece] ^= (1ull << sq);
    board[sq] = piece;
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
    int bk = get_lsb(pieces[10]), wk = get_lsb(pieces[11]);
    for (u64 bb{pieces[0]}; bb;) {
        int sq = pop_lsb(bb);
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
        int sq = pop_lsb(bb);
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
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][2][sq] + full_king[1][wk][2][sq];
        u64 moves = knight_attacks[sq] & ~black_pieces;
        eval -= knight_mobility[popcount(moves)] + knight_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
    }
    for (u64 bb{pieces[3]}; bb;) {
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][3][sq] + full_king[1][wk][3][sq];
        u64 moves = knight_attacks[sq] & ~white_pieces;
        eval += knight_mobility[popcount(moves)] + knight_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
    }
    for (u64 bb{pieces[4]}; bb;) {
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][4][sq] + full_king[1][wk][4][sq];
        u64 moves = bishop_attacks(occupied & ~pieces[8], sq) & ~black_pieces;
        eval -= bishop_mobility[popcount(moves)] + bishop_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
    }
    for (u64 bb{pieces[5]}; bb;) {
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][5][sq] + full_king[1][wk][5][sq];
        u64 moves = bishop_attacks(occupied & ~pieces[9], sq) & ~white_pieces;
        eval += bishop_mobility[popcount(moves)] + bishop_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
    }
    //bishop pair eval
    if (popcount(pieces[4]) >= 2) eval -= bishop_pair;
    if (popcount(pieces[5]) >= 2) eval += bishop_pair;
    for (u64 bb{pieces[6]}; bb;) {
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][6][sq] + full_king[1][wk][6][sq];
        u64 moves = rook_attacks(occupied & ~pieces[8], sq) & ~black_pieces;
        eval -= rook_mobility[popcount(moves)] + rook_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
    }
    for (u64 bb{pieces[7]}; bb;) {
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][7][sq] + full_king[1][wk][7][sq];
        u64 moves = rook_attacks(occupied & ~pieces[9], sq) & ~white_pieces;
        eval += rook_mobility[popcount(moves)] + rook_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
    }
    for (u64 bb{pieces[8]}; bb;) {
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][8][sq] + full_king[1][wk][8][sq];
        u64 moves = queen_attacks(occupied, sq) & ~black_pieces;
        eval -= queen_mobility[popcount(moves)] + queen_forward_mobility[popcount(moves & forward_mask[0][sq >> 3])];
    }
    for (u64 bb{pieces[9]}; bb;) {
        int sq = pop_lsb(bb);
        eval += full_king[0][bk][9][sq] + full_king[1][wk][9][sq];
        u64 moves = queen_attacks(occupied, sq) & ~white_pieces;
        eval += queen_mobility[popcount(moves)] + queen_forward_mobility[popcount(moves & forward_mask[1][sq >> 3])];
    }
    eval_stack[ply] = ((2 * side_to_move - 1) * (static_cast<s16>(eval >> 16) * phase + static_cast<s16>(eval & 0xFFFF) * (24 - phase)) / 24) + phase / 2;
    return eval_stack[ply];
}

void Position::load(std::vector<int> position, bool stm) {
    ply = 0;
    for (int sq{0}; sq<position.size(); ++sq) fill_sq<false>(sq, position[sq]);
    side_to_move = stm;
    zobrist_update();
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
    if (castling_rights[ply] & 8) fen += "K";
    if (castling_rights[ply] & 4) fen += "Q";
    if (castling_rights[ply] & 2) fen += "k";
    if (castling_rights[ply] & 1) fen += "q";
    if (castling_rights[ply] == 0) fen += "-";
    if (enpassant_square[ply] == 64) fen += " -";
    else fen += " " + square_names[enpassant_square[ply]];
    fen += " 0 1";
    return fen;
}

bool Position::load_fen(std::string fen_pos, std::string fen_stm, std::string fen_castling, std::string fen_ep, std::string fen_hmove_clock = "0", std::string fen_fmove_counter = "1") {
    int sq = 0;
    ply = 0;
    for (int i{}; i<64; ++i) fill_sq<false>(i, empty_square);
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
    if (fen_stm == "w") side_to_move = true;
    else if (fen_stm == "b") side_to_move = false;
    else return false;
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
    for (int i{0}; i<64; ++i) zobrist_pieces[12][i] = 0;
    zobrist_black = rand64();
    for (int i{0}; i<64; ++i) zobrist_enpassant[i] = rand64();
    for (int i{0}; i<16; ++i) zobrist_castling[i] = rand64();
    zobrist_enpassant[64] = 0;
    zobrist_update();
}

void Position::zobrist_update() {
    hash[ply] = 0;
    for (int i{0}; i<64; ++i) hash[ply] ^= zobrist_pieces[board[i]][i];
    if (!side_to_move) hash[ply] ^= zobrist_black;
    hash[ply] ^= zobrist_castling[castling_rights[ply]];
    hash[ply] ^= zobrist_enpassant[enpassant_square[ply]];
}
