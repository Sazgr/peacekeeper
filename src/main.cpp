/*
Peacekeeper Chess Engine
    Copyright (C) 2022-2023  Kyle Zhang
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "main.h"
#include "movelist.h"
#include "uci.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <string.h>
#include <thread>
#include <vector>

u64 nodes{0};
Move pv_table[128][128];

std::ifstream infile ("peacekeeper/logs/input.txt");
std::ofstream debug ("logs/debug.txt");
std::ostream& out = std::cout;
std::istream& in = std::cin;

bool debug_mode{false};

u64 beta_cuts;
u64 cut_num;

u64 tt_queries;
u64 tt_hits;
u64 tt_moves;
u64 tt_cutoffs;

u64 main_nodes;
u64 extended;
u64 red_attempts;
u64 reduced;
u64 pruned;
u64 null_attempts;
u64 nulled;

int main(int argc, char *argv[]) {
    Move move{};
    Movelist movelist;
    std::string movestring;
    Position position;
    Hashtable hash{1};
    Move_order_tables move_order{};
    Stop_timer timer{0, 0, 0};
    int move_overhead = 5;
    std::atomic<bool>& stop = timer.stop;
    std::string command, token;
    std::vector<std::string> tokens;
    std::cout << "peacekeeper by sazgr" << std::endl;
    if (argc > 1 && std::string{argv[1]} == "bench") {
        const static std::array<std::string, 20> fens = {
            "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
            "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
            "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
            "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
            "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
            "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
            "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
            "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
            "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
            "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
            "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
            "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
            "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
            "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
            "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
            "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
            "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
            "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
            "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
            "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34"
        };
        u64 total_nodes = 0;
        double total_time = 0.0;
        for (std::string fen : fens) {
            tokens.clear();
            std::istringstream parser(fen);
            while (parser >> token) {tokens.push_back(token);}
            timer.reset(0, 0, 0, 11);
            position.load_fen(tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5]);
            iterative_deepening(position, timer, hash, move_order, move, false);
            total_nodes += nodes;
            total_time += timer.elapsed();
        }
        out << total_nodes << " nodes " << static_cast<int>(total_nodes / total_time) << " nps" << std::endl;
        return 0;
    } else if (argc > 1) {
        std::cout << "unsupported command-line argument \"" << argv[1] << "\"" << std::endl;
    }
    while (true) {
        getline(in, command);
        tokens.clear();
        std::istringstream parser(command);
        while (parser >> token) {tokens.push_back(token);}
        if (tokens.size() == 0) {continue;}
        if (tokens[0] == "debug") {
            if (tokens.size() >= 2 && tokens[1] == "on") debug_mode = true;
            if (tokens.size() >= 2 && tokens[1] == "off") debug_mode = false;
        }
        if (tokens[0] == "datagen") {
            std::cout << "indexing openings..." << std::endl;
            srand(time(0));
            std::ifstream fin (tokens[1]);
            std::ofstream fout (tokens[2], std::ios_base::app);
            std::vector<std::array<std::string, 6>> openings{};
            std::array<std::string, 6> opening;
            while (fin) {
                fin >> opening[0] >> opening[1] >> opening[2] >> opening[3] >> opening[4] >> opening[5];
                if (fin) openings.push_back(opening);
            }
            std::cout << "playing games..." << std::endl;
            std::string result_string;
            std::vector<std::string> buffer{};
            while (true) {
                buffer.clear();
                int id = rand() % openings.size();
                position.load_fen(openings[id][0], openings[id][1], openings[id][2], openings[id][3], openings[id][4], openings[id][5]);
                hash.clear();
                move_order.reset();
                while (true) {
                    if (!position.count_legal_moves()) {
                        if (position.check()) {
                            if (position.side_to_move) result_string = " c9 \"0-1\";";
                            else result_string = " c9 \"1-0\";";
                        }
                        else result_string = " c9 \"1/2-1/2\";";
                        break;
                    }
                    if (position.draw(2)) {
                        result_string = " c9 \"1/2-1/2\";";
                        if (position.halfmove_clock[position.ply] >= 100) for (int i{}; i<20; ++i) buffer.pop_back();
                        break;
                    }
                    if (position.eval_phase() <= 1 && !position.board[0] && !position.board[1]) {
                        result_string = " c9 \"1/2-1/2\";";
                        break;
                    } 
                    timer.reset(200, 50, 0, 0);
                    int score = iterative_deepening(position, timer, hash, move_order, move, false);
                    if (abs(score) > 18000) {
                        if ((score > 18000) == (position.side_to_move)) result_string = " c9 \"1-0\";";
                        else result_string = " c9 \"0-1\";";
                        break;
                    }
                    if (move.captured() == 12 && (move.flag() == none || move.flag() == q_castling || move.flag() == k_castling) && !position.draw(1)) buffer.push_back(position.export_fen());
                    position.make_move(move);
                }
                if (result_string == " c9 \"0-1\";") std::cout << "0";
                if (result_string == " c9 \"1/2-1/2\";") std::cout << "=";
                if (result_string == " c9 \"1-0\";") std::cout << "1";
                for (std::string fen : buffer) {
                    fout << fen << result_string << std::endl;
                }
            }
        }
        if (tokens[0] == "eval") {
            out << position << "PSQT: " << position.static_eval() << std::endl;
        }
        if (tokens[0] == "go") {
            if (std::find(tokens.begin(), tokens.end(), "infinite") != tokens.end()) {
                timer.reset();
                std::thread search{iterative_deepening, std::ref(position), std::ref(timer), std::ref(hash), std::ref(move_order), std::ref(move), true};
                search.detach();
                continue;
            }
            int movetime = 0;
            int depth = 0;
            int nodes = 0;
            bool calculate = false;
            int wtime = 0;
            int btime = 0;
            int winc = 0;
            int binc = 0;
            int movestogo = 0;
            if (std::find(tokens.begin(), tokens.end(), "movetime") != tokens.end()) {movetime = stoi(*(++std::find(tokens.begin(), tokens.end(), "movetime")));}
            if (std::find(tokens.begin(), tokens.end(), "depth") != tokens.end()) {depth = stoi(*(++std::find(tokens.begin(), tokens.end(), "depth")));}
            if (std::find(tokens.begin(), tokens.end(), "nodes") != tokens.end()) {nodes = stoi(*(++std::find(tokens.begin(), tokens.end(), "nodes")));}
            if (std::find(tokens.begin(), tokens.end(), "wtime") != tokens.end()) {
                calculate = true;
                wtime = stoi(*(++std::find(tokens.begin(), tokens.end(), "wtime")));
            }
            if (std::find(tokens.begin(), tokens.end(), "btime") != tokens.end()) {
                calculate = true;
                btime = stoi(*(++std::find(tokens.begin(), tokens.end(), "btime")));
            }
            if (std::find(tokens.begin(), tokens.end(), "winc") != tokens.end()) {winc = stoi(*(++std::find(tokens.begin(), tokens.end(), "winc")));}
            if (std::find(tokens.begin(), tokens.end(), "binc") != tokens.end()) {binc = stoi(*(++std::find(tokens.begin(), tokens.end(), "binc")));}
            if (std::find(tokens.begin(), tokens.end(), "movestogo") != tokens.end()) {movestogo = stoi(*(++std::find(tokens.begin(), tokens.end(), "movestogo"))) + 1;}
            int mytime;
            if (movetime == 0 && calculate == true) {
                mytime = position.side_to_move ? wtime : btime;
                int myinc{position.side_to_move ? winc : binc};
                if (movestogo == 0 || movestogo > std::max(30, (50 - position.ply / 5))) {movestogo = std::max(30, (50 - position.ply / 5));} //estimated number of moves until fresh time or end of game
                movetime = 0.75 * (mytime / movestogo + winc); //time usable in terms of time remaining and increment
                movetime -= move_overhead; //accounting for lag, network delay, etc
                movetime = std::max(1, movetime); //no negative movetime
            }
            timer.reset(calculate ? std::max(1, std::min(mytime - move_overhead, 4 * movetime)) : movetime, calculate ? movetime : 0, nodes, depth);
            std::thread search{iterative_deepening, std::ref(position), std::ref(timer), std::ref(hash), std::ref(move_order), std::ref(move), true};
            search.detach();
        }
        if (tokens[0] == "isready") {out << "readyok" << std::endl;}
        if (tokens[0] == "perft") {
            int depth = 1;
            if (tokens.size() >= 2) {depth = stoi(tokens[1]);}
            Timer timer;
            timer.reset();
            u64 result = (position.side_to_move ? perft_f<true>(position, depth) : perft_f<false>(position, depth));
            double end = timer.elapsed();
            out << "info nodes " << result << " time " << static_cast<int>(end * 1000) << " nps " << static_cast<int>(result / end) << std::endl;
        }
        if (tokens[0] == "makemove") {
            if (tokens.size() == 1) continue;
            position.parse_move(move, tokens[1]);
            position.make_move(move);
        }
        if (tokens[0] == "perftsplit") {
            int depth = 1;
            if (tokens.size() >= 2) {depth = stoi(tokens[1]);}
            std::vector<std::pair<Move, int>> list{};
            int result = perft_split(position, depth, list);
            out << result << '\n';
            std::sort(list.begin(), list.end(), [] (std::pair<Move, int> entry1, std::pair<Move, int> entry2) {return (entry1.first.start() < entry2.first.start()) || (entry1.first.start() == entry2.first.start() && entry1.first.end() < entry2.first.end());});
            for (int i{0}; i<list.size(); ++i) {
                out << list[i].first << ' ' << list[i].second << '\n';
            }
        }
        if (tokens[0] == "position") {
            if (tokens.size() == 1) continue;
            if (tokens[1] == "startpos") position.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
            else if (tokens[1] == "kiwipete") position.load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-", "0", "1");
            else if (tokens[1] == "fen") position.load_fen(tokens[2], tokens[3], tokens[4], tokens[5], tokens[6], tokens[7]);
            if (std::find(tokens.begin(), tokens.end(), "moves") != tokens.end()) {
                for (auto iter = ++std::find(tokens.begin(), tokens.end(), "moves"); iter != tokens.end(); ++iter) {
                    position.parse_move(move, *iter);
                    position.make_move(move);
                }
            }
        }
        if (tokens[0] == "quit") {
            stop = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); //wait 200 milliseconds to make sure that any ongoing searches stop before quitting
            return 0;
        }
        if (tokens[0] == "setoption" && tokens[1] == "name") {
            if (tokens.size() >= 5 && tokens[2] == "Hash" && tokens[3] == "value") {hash.resize(stoi(tokens[4]));}
            if (tokens.size() >= 6 && tokens[2] == "Move" && tokens[3] == "Overhead" && tokens[4] == "value") {move_overhead = stoi(tokens[5]);}
#ifdef SPSA
            if (tokens.size() >= 5 && tokens[2] == "futility_multiplier" && tokens[3] == "value") {
                futility_multiplier = 0.1 * stoi(tokens[4]);
                for (int i{}; i<24; ++i) futile_margins[i] = futility_multiplier * std::pow(i + 4, futility_power);
            }
            if (tokens.size() >= 5 && tokens[2] == "futility_power" && tokens[3] == "value") {
                futility_power = 0.01 * stoi(tokens[4]);
                for (int i{}; i<24; ++i) futile_margins[i] = futility_multiplier * std::pow(i + 4, futility_power);
            }
            if (tokens.size() >= 5 && tokens[2] == "see_noisy_constant" && tokens[3] == "value") {
                see_noisy_constant = 0.1 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "see_noisy_linear" && tokens[3] == "value") {
                see_noisy_linear = 0.1 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "see_noisy_quadratic" && tokens[3] == "value") {
                see_noisy_quadratic = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "see_quiet_constant" && tokens[3] == "value") {
                see_quiet_constant = 0.1 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "see_quiet_linear" && tokens[3] == "value") {
                see_quiet_linear = 0.1 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "see_quiet_quadratic" && tokens[3] == "value") {
                see_quiet_quadratic = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "node_timescale_base" && tokens[3] == "value") {
                node_timescale_base = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "node_timescale_div" && tokens[3] == "value") {
                node_timescale_div = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "aspiration_beta_timescale" && tokens[3] == "value") {
                aspiration_beta_timescale = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "tc_stability_0" && tokens[3] == "value") {
                tc_stability[0] = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "tc_stability_1" && tokens[3] == "value") {
                tc_stability[1] = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "tc_stability_2" && tokens[3] == "value") {
                tc_stability[2] = 0.01 * stoi(tokens[4]);
            }
            if (tokens.size() >= 5 && tokens[2] == "tc_stability_3" && tokens[3] == "value") {
                tc_stability[3] = 0.01 * stoi(tokens[4]);
            }
#endif
        }
        if (tokens[0] == "see") {
            position.parse_move(move, tokens[1]);
            std::cout << see(position, move, -107);
        }
        if (tokens[0] == "stop") {stop = true;}
        if (tokens[0] == "uci") {print_info(out);}
        if (tokens[0] == "ucinewgame") {
            hash.clear();
            move_order.reset();
        }
    }
    return 0;
}

u64 perft(Position& position, int depth) {
    if (depth == 1) {
        return position.count_legal_moves();
    }
    u64 total{};
    Movelist movelist;
    position.legal_moves(movelist);
    for (int i{}; i<movelist.size(); ++i) {
        position.make_move(movelist[i]);
        total += perft(position, depth - 1);
        position.undo_move(movelist[i]);
    }
    return total;
}

u64 perft_split(Position& position, int depth, std::vector<std::pair<Move, int>>& list) {
    if (depth == 1) {
        Movelist movelist;
        position.legal_moves(movelist);
        for (int i{}; i < movelist.size(); ++i) {
            out << movelist[i] << '\n';
        }
        return movelist.size();
    } else {
        u64 total{};
        Movelist movelist;
        position.legal_moves(movelist);
        for (int i{}; i < movelist.size(); ++i) {
            position.make_move(movelist[i]);
            int result = perft(position, depth - 1);
            list.push_back({movelist[i], result});
            total += result;
            position.undo_move(movelist[i]);
        }
        return total;
    }
}

template <bool side> u64 perft_f(Position& position, int depth) {
    if (depth == 1) {
        return position.count_legal<all, side>();
    }
    u64 total{};
    Movelist movelist;
    position.gen_legal<all, side>(movelist);
    for (int i{}; i<movelist.size(); ++i) {
        position.make_move(movelist[i]);
        total += perft_f<!side>(position, depth - 1);
        position.undo_move(movelist[i]);
    }
    return total;
}

bool see(Position& position, Move move, const int threshold) {
    int to = move.end();
    int from = move.start();
    int target = position.board[to];
    //making the move and not losing it must beat the threshold
    int value = see_value[target >> 1] - threshold;
    if (move.flag() > none && move.flag() < q_castling) return true;
    if (value < 0) return false;
    int attacker = position.board[from];
    //trivial if we still beat the threshold after losing the piece
    value -= see_value[attacker >> 1];
    if (value >= 0)
        return true;
    //it doesn't matter if the to square is occupied or not
    u64 occupied = position.occupied ^ (1ull << from) ^ (1ull << to);
    u64 attackers = position.attacks_to_square(occupied, to);
    u64 bishops = position.pieces[4] | position.pieces[5] | position.pieces[8] | position.pieces[9];
    u64 rooks = position.pieces[6] | position.pieces[7] | position.pieces[8] | position.pieces[9];
    int side = (attacker & 1) ^ 1;
    u64 side_pieces[2] = {position.pieces[0] | position.pieces[2] | position.pieces[4] | position.pieces[6] | position.pieces[8] | position.pieces[10],
                          position.pieces[1] | position.pieces[3] | position.pieces[5] | position.pieces[7] | position.pieces[9] | position.pieces[11]};
    //make captures until one side runs out, or fail to beat threshold
    while (true) {
        //remove used pieces from attackers
        attackers &= occupied;
        u64 my_attackers = attackers & side_pieces[side];
        if (!my_attackers) {
            break;
        }
        //pick next least valuable piece to capture with
        int piece_type;
        for (piece_type = 0; piece_type < 6; ++piece_type) {
            if (my_attackers & position.pieces[2 * piece_type + side]) break;
        } 
        side = !side;
        value = -value - 1 - see_value[piece_type];
        //value beats threshold, or can't beat threshold (negamaxed)
        if (value >= 0) {
            if (piece_type == 5 && (attackers & side_pieces[side]))
                side = !side;
            break;
        }
        //remove the used piece from occupied
        occupied ^= (1ull << (get_lsb(my_attackers & (position.pieces[2 * piece_type] | position.pieces[2 * piece_type + 1]))));
        if (piece_type == 0 || piece_type == 2 || piece_type == 4)
            attackers |= position.bishop_attacks(occupied, to) & bishops;
        if (piece_type == 3 || piece_type == 4)
            attackers |= position.rook_attacks(occupied, to) & rooks;
    }
    return side != (attacker & 1);
}

int quiescence(Position& position, Stop_timer& timer, Hashtable& table, int alpha, int beta, Search_stack* ss) {
    if (timer.stopped() || (!(nodes & 8191) && timer.check(nodes))) return 0;
    if (position.check()) {
        int result = -20000;
        Movelist movelist;
        position.legal_moves(movelist);
        if (movelist.size() == 0) return -20000 + ss->ply;
        for (int i = 0; i < movelist.size(); ++i) movelist[i].add_sortkey(movelist[i].evade_order());
        movelist.sort(0, movelist.size());
        for (int i = 0; i < movelist.size(); ++i) {
            position.make_move(movelist[i]);
            ss->move = movelist[i];
            ++nodes;
            (ss + 1)->ply = ss->ply + 1;
            result = -quiescence(position, timer, table, -beta, -alpha, ss + 1);
            position.undo_move(movelist[i]);
            if (result > alpha) {
                alpha = result;
                if (alpha >= beta) return alpha;
            }
        }
        return alpha;
    } else {
        table.prefetch(position.hashkey());
        int static_eval = position.static_eval();
        if (static_eval >= beta) return static_eval; //if the position is already so good, cutoff immediately
        if (alpha < static_eval) alpha = static_eval;
        int old_alpha{alpha};
        Move bestmove{};
        Element entry = table.query(position.hashkey());
        ++tt_queries;
        if (entry.type() != tt_none && entry.full_hash == position.hashkey()) ++tt_hits;
        if (entry.type() != tt_none && entry.full_hash == position.hashkey() && no_mate(entry.score(), entry.score()) && (entry.type() == tt_exact || (entry.type() == tt_alpha && entry.score() <= alpha) || (entry.type() == tt_beta && entry.score() >= beta))) {
            ++tt_cutoffs;
            return entry.score();
        }
        int result = -20000;
        Move hash_move = entry.bestmove();
        bool hash_move_usable = entry.type() != tt_none && entry.full_hash == position.hashkey() && !hash_move.is_null() && hash_move.captured() != 12 && position.board[hash_move.start()] == hash_move.piece() && position.board[hash_move.end()] == hash_move.captured();
        if (hash_move_usable) {//searching best move from hashtable
            if (!(delta_pruning && static_eval + hash_move.gain() + futile_margins[0] < alpha)) { //delta pruning
                position.make_move(hash_move);
                ss->move = hash_move;
                ++nodes;
                (ss + 1)->ply = ss->ply + 1;
                result = -quiescence(position, timer, table, -beta, -alpha, ss + 1);
                position.undo_move(hash_move);
                if (result > alpha) {
                    alpha = result;
                    bestmove = hash_move;
                    if (alpha >= beta) {
                        if (!timer.stopped()) table.insert(position.hashkey(), alpha, tt_beta, bestmove, 0);
                        return alpha;
                    }
                }
            }
        }
        Movelist movelist;
        position.legal_noisy(movelist);
        for (int i = 0; i < movelist.size(); ++i) movelist[i].add_sortkey(movelist[i].mvv_lva());
        movelist.sort(0, movelist.size());
        for (int i = 0; i < movelist.size(); ++i) {
            if constexpr (delta_pruning) if (static_eval + movelist[i].gain() + futile_margins[0] < alpha) break; //delta pruning
            if (!see(position, movelist[i], -107)) continue;
            position.make_move(movelist[i]);
            ss->move = movelist[i];
            ++nodes;
            (ss + 1)->ply = ss->ply + 1;
            result = -quiescence(position, timer, table, -beta, -alpha, ss + 1);
            position.undo_move(movelist[i]);
            if (result > alpha) {
                alpha = result;
                bestmove = movelist[i];
                if (alpha >= beta) {
                    if (!timer.stopped()) table.insert(position.hashkey(), alpha, tt_beta, bestmove, 0);
                    return alpha;
                }
            }
        }
        if (!timer.stopped()) table.insert(position.hashkey(), alpha, ((alpha > old_alpha)?tt_exact:tt_alpha), bestmove, 0);
        return alpha;
    }
}

int pvs(Position& position, Stop_timer& timer, Hashtable& table, Move_order_tables& move_order, int depth, int alpha, int beta, Search_stack* ss) {
    bool is_root = (ss->ply == 0);
    bool is_pv = (beta - alpha) != 1;
    if (timer.stopped() || (!(nodes & 8191) && timer.check(nodes, 0))) return 0;
    if (depth <= 0) {
        return quiescence(position, timer, table, alpha, beta, ss);
    }
    if (depth == 1 && is_pv) pv_table[ss->ply + 1][0] = Move{};
    if (position.draw(ss->ply > 2 ? 1 : 2)) {
        pv_table[ss->ply][0] = Move{};
        return (ss->ply & 1) ? std::max(0, 3 * position.eval_phase() - 12) : std::min(0, 12 - 3 * position.eval_phase());
    }
    table.prefetch(position.hashkey());
    bool in_check = position.check();//condition for NMP, futility, and LMR
    int static_eval = position.static_eval();
    ss->static_eval = static_eval;
    int move_num{0};
    int result{};
    int old_alpha{alpha};
    int nodes_before; //used for node time management
    int reduce_all{1};
    int reduce_this{};
    Move bestmove{};
    bool improving = !in_check && (ss - 2)->static_eval != -20001 && ss->static_eval > (ss - 2)->static_eval;
    if constexpr (static_null_move) if (depth < 6 && !(ss - 1)->move.is_null() && !is_pv && !in_check && beta > -18000 && (static_eval - futile_margins[depth - improving] >= beta)) {
        ++pruned;
        return static_eval - futile_margins[depth - improving];
    }
    Element entry = table.query(position.hashkey());
    ++tt_queries;
    if (entry.type() != tt_none && entry.full_hash == position.hashkey()) ++tt_hits;
    if (!is_pv && entry.type() != tt_none && entry.full_hash == position.hashkey() && entry.depth() >= depth && no_mate(entry.score(), entry.score()) && (entry.type() == tt_exact || (entry.type() == tt_alpha && entry.score() <= alpha) || (entry.type() == tt_beta && entry.score() >= beta))) {
        ++tt_cutoffs;
        return entry.score();
    }
    if constexpr (null_move_pruning) if (depth > 2 && !(ss - 1)->move.is_null() && !is_pv && !in_check && beta > -18000 && static_eval > beta && (position.eval_phase() >= 4)) {
        position.make_null();
        ss->move = Move{};
        ++nodes;
        ++null_attempts;
        (ss + 1)->ply = ss->ply + 1;
        result = -pvs(position, timer, table, move_order, std::max(1, depth - reduce_all - static_cast<int>(2.19999999 + depth / 4.0 + improving + std::sqrt(static_eval - beta) / 12.0)), -beta, -beta + 1, ss + 1);
        position.undo_null();
        if (!timer.stopped() && result >= beta) {
            //if (!timer.stopped) table.insert(position.hashkey(), result, tt_beta, bestmove, depth);
            ++nulled;
            return result;
        }
    }
    if constexpr (check_extensions) if (in_check) {++extended; reduce_all -= 1;} //check extension
    Move hash_move = entry.bestmove();
    bool hash_move_usable = entry.type() != tt_none && entry.full_hash == position.hashkey() && !hash_move.is_null() && position.board[hash_move.start()] == hash_move.piece() && position.board[hash_move.end()] == hash_move.captured();
    if constexpr (internal_iterative_reduction) if (depth >= 6 && !hash_move_usable) reduce_all += 1;
    //Stage 1 - Hash Move
    if (hash_move_usable) {//searching best move from hashtable
        position.make_move(hash_move);
        ss->move = hash_move;
        if (is_root) nodes_before = nodes;
        ++nodes;
        ++move_num;
        ++main_nodes;
        ++tt_moves;
        (ss + 1)->ply = ss->ply + 1;
        result = -pvs(position, timer, table, move_order, depth - reduce_all, -beta, -alpha, ss + 1);
        position.undo_move(hash_move);
        if (is_root) nodes_used[hash_move.start()][hash_move.end()] += nodes - nodes_before;
        if (!timer.stopped() && result > alpha) {
            alpha = result;
            bestmove = hash_move;
            if (is_pv) {
                pv_table[ss->ply][0] = bestmove;
                memcpy(&pv_table[ss->ply][1], &pv_table[ss->ply + 1][0], sizeof(Move) * 127);
            }
            if (alpha >= beta) {
                if constexpr (history_heuristic) if (bestmove.captured() == 12) {
                    move_order.history_edit(bestmove.piece(), bestmove.end(), depth * depth, true);
                    move_order.continuation_edit((ss - 2)->move, bestmove, depth * depth, true);
                    move_order.continuation_edit((ss - 1)->move, bestmove, depth * depth, true);  
                }
                table.insert(position.hashkey(), alpha, tt_beta, bestmove, depth);
                ++beta_cuts;
                cut_num += move_num;
                return alpha;
            }
        }
    }
    Movelist movelist;
    position.legal_noisy(movelist);
    for (int i = 0; i < movelist.size(); ++i) {
        movelist[i].add_sortkey(movelist[i].mvv_lva());
    }
    movelist.sort(0, movelist.size());
    bool can_fut_prune = !in_check && no_mate(alpha, beta) && depth < 6;
    //Stage 2 - Captures
    for (int i{}; i < movelist.size(); ++i) {
        if (hash_move_usable && movelist[i] == hash_move) continue; //continuing if we already searched the hash move
        if (!see(position, movelist[i], -see_noisy_constant - see_noisy_linear * depth - see_noisy_quadratic * depth * depth)) continue;
        position.make_move(movelist[i]);
        ss->move = movelist[i];
        if (is_root) nodes_before = nodes;
        ++nodes;
        ++move_num;
        ++main_nodes;
        (ss + 1)->ply = ss->ply + 1;
        if (depth == 1 || !is_pv || move_num == 1) {
            result = -pvs(position, timer, table, move_order, depth - reduce_all, -beta, -alpha, ss + 1);
        } else {
            result = -pvs(position, timer, table, move_order, depth - reduce_all, -alpha-1, -alpha, ss + 1);
            if (alpha < result && result < beta) {
                result = -pvs(position, timer, table, move_order, depth - reduce_all, -beta, -alpha, ss + 1);
            }
        }
        position.undo_move(movelist[i]);
        if (is_root) nodes_used[movelist[i].start()][movelist[i].end()] += nodes - nodes_before;
        if (!timer.stopped() && result > alpha) {
            alpha = result;
            bestmove = movelist[i];
            if (is_pv) {
                pv_table[ss->ply][0] = bestmove;
                memcpy(&pv_table[ss->ply][1], &pv_table[ss->ply + 1][0], sizeof(Move) * 127);
            }
            if (alpha >= beta) {
                table.insert(position.hashkey(), alpha, tt_beta, bestmove, depth);
                ++beta_cuts;
                cut_num += move_num;
                return alpha;
            }
        }
    }
    position.legal_quiet(movelist);
    for (int i = 0; i < movelist.size(); ++i) {
        int score{};
        if constexpr (history_heuristic) {
            score += move_order.history_value(movelist[i].piece(), movelist[i].end());
            score += move_order.continuation_value((ss - 2)->move, movelist[i]);
            score += move_order.continuation_value((ss - 1)->move, movelist[i]);
        }
        if constexpr (killer_heuristic) {
            if (movelist[i] == move_order.killer_move(ss->ply, 0)) score += 1600;
            if (movelist[i] == move_order.killer_move(ss->ply, 1)) score += 800;
            if (ss->ply > 2 && movelist[i] == move_order.killer_move(ss->ply - 2, 0)) score += 400;
            if (ss->ply > 2 && movelist[i] == move_order.killer_move(ss->ply - 2, 1)) score += 200;
        }
        movelist[i].add_sortkey(score);
    }
    movelist.sort(0, movelist.size());
    //Stage 3 - Quiet Moves
    for (int i{}; i < movelist.size(); ++i) {
        if (hash_move_usable && movelist[i] == hash_move) continue; //continuing if we already searched the hash move
        if (!see(position, movelist[i], -see_quiet_constant - see_quiet_linear * depth - see_quiet_quadratic * depth * depth)) continue;
        position.make_move(movelist[i]);
        ss->move = movelist[i];
        bool gives_check = position.check();
        //Futility Pruning
        if constexpr (futility_pruning) if (can_fut_prune && (static_eval + futile_margins[depth] - late_move_margin(depth, move_num) < alpha) && move_num != 0 && !gives_check) {
            position.undo_move(movelist[i]);
            ++pruned;
            continue;
        }
        if (is_root) nodes_before = nodes;
        ++nodes;
        ++move_num;
        ++main_nodes;
        (ss + 1)->ply = ss->ply + 1;
        //Late Move Reductions
        reduce_this = 0;
        if constexpr (late_move_reductions) if (depth > 2 && move_num >= 4 && !in_check && !gives_check) {
            reduce_this = lmr_reduction(is_pv, depth, move_num);
        }
        if (move_num == 1) {
            result = -pvs(position, timer, table, move_order, depth - reduce_all, -beta, -alpha, ss + 1);
        } else {
            if (reduce_this) { //try a reduced search
                ++red_attempts;
            }
            result = -pvs(position, timer, table, move_order, depth - reduce_all - reduce_this, -alpha - 1, -alpha, ss + 1);
            if (reduce_this) {
                if (alpha < result) {
                    result = -pvs(position, timer, table, move_order, depth - reduce_all, -alpha - 1, -alpha, ss + 1);
                } else {
                    ++reduced;
                }
            }
            if (alpha < result && result < beta && is_pv) {
                result = -pvs(position, timer, table, move_order, depth - reduce_all,  -beta, -alpha, ss + 1);
            }
        }
        position.undo_move(movelist[i]);
        if (is_root) nodes_used[movelist[i].start()][movelist[i].end()] += nodes - nodes_before;
        if (!timer.stopped() && result > alpha) {
            alpha = result;
            bestmove = movelist[i];
            if (is_pv) {
                pv_table[ss->ply][0] = bestmove;
                memcpy(&pv_table[ss->ply][1], &pv_table[ss->ply + 1][0], sizeof(Move) * 127);
            }
            if (alpha >= beta) {
                if constexpr (history_heuristic) for (int j{0}; j<i; ++j) {
                    move_order.history_edit(movelist[j].piece(), movelist[j].end(), depth * depth, false);
                    move_order.continuation_edit((ss - 2)->move, movelist[j], depth * depth, false);
                    move_order.continuation_edit((ss - 1)->move, movelist[j], depth * depth, false);   
                }
                if constexpr (history_heuristic) {
                    move_order.history_edit(bestmove.piece(), bestmove.end(), depth * depth, true);
                    move_order.continuation_edit((ss - 2)->move, bestmove, depth * depth, true);
                    move_order.continuation_edit((ss - 1)->move, bestmove, depth * depth, true);  
                }
                table.insert(position.hashkey(), alpha, tt_beta, bestmove, depth);
                if constexpr (killer_heuristic) move_order.killer_add(bestmove, ss->ply);
                ++beta_cuts;
                cut_num += move_num;
                return alpha;
            }
        }
    }
    if (move_num == 0) {
        pv_table[ss->ply][0] = Move{};
        if (in_check) return -20000 + ss->ply;
        else return 0;
    }
    if (!timer.stopped()) table.insert(position.hashkey(), alpha, ((alpha > old_alpha)?tt_exact:tt_alpha), bestmove, depth);
    return timer.stopped() ? 0 : alpha;
}

int iterative_deepening(Position& position, Stop_timer& timer, Hashtable& table, Move_order_tables& move_order, Move& bestmove, bool output) {
    if constexpr (history_heuristic) move_order.age();
    table.age();
    Movelist movelist;
    Search_stack search_stack[96];
    search_stack[2].ply = 0;
    position.legal_moves(movelist);
    if (movelist.size() == 0) return 0;
    else {
        int alpha = -20000;
        int beta = 20000;
        double time_scale = 1;
        int nodes_before = 0;
        nodes = 0;
        beta_cuts = 0;
        cut_num = 0;
        tt_queries = 0;
        tt_hits = 0;
        tt_moves = 0;
        tt_cutoffs = 0;
        main_nodes = 0;
        extended = 0;
        red_attempts = 0;
        reduced = 0;
        pruned = 0;
        null_attempts = 0;
        nulled = 0;
        int depth{1};
        int last_score, result;
        int stability = 0;
        bestmove = movelist[0];
        pv_table[0][0] = Move{};
        for (int i{}; i<64; ++i) {
            for (int j{}; j<64; ++j) {
                nodes_used[i][j] == 0;
            }
        }
        for (; depth <= 64;) {
            result = pvs(position, timer, table, move_order, depth, alpha, beta, &search_stack[2]);
            if (alpha < result && result < beta) {
                if (!timer.stopped()) last_score = result;
                if (output) print_uci(out, last_score, depth, nodes, static_cast<int>(nodes/timer.elapsed()), static_cast<int>(timer.elapsed()*1000), pv_table[0]);
                ++depth;
                if (pv_table[0][0] == bestmove) {
                    stability = std::min(stability + 1, 3);
                } else {
                    stability = 0;
                }
                if (!pv_table[0][0].is_null()) bestmove = pv_table[0][0];
                //time_scale = (node_timescale_base - static_cast<double>(nodes_used[bestmove.start()][bestmove.end()]) / (nodes)) / node_timescale_div;
                //nodes_before = nodes;
                time_scale = tc_stability[stability];
                if (timer.check(nodes, depth)) {break;}
                if (!bestmove.is_null() && timer.check(nodes, depth, true, (movelist.size() == 1 ? 0.5 : 1) * time_scale)) {break;}
                alpha = last_score - aspiration_bounds[0];
                beta = last_score + aspiration_bounds[0];
                continue;
            }
            if (result <= alpha) {
                //no time checks here because we failed low, we allow for some extra time
                if (alpha == last_score - aspiration_bounds[0]) alpha = last_score - aspiration_bounds[1];
                else if (alpha == last_score - aspiration_bounds[1]) alpha = last_score - aspiration_bounds[2];
                else alpha = -20000;
            } 
            if (result >= beta) {
                if (!pv_table[0][0].is_null()) bestmove = pv_table[0][0];
                if (!bestmove.is_null() && timer.check(nodes, depth, true, (movelist.size() == 1 ? 0.5 : 1) * time_scale * aspiration_beta_timescale)) {break;}
                if (beta == last_score + aspiration_bounds[0]) beta = last_score + aspiration_bounds[1];
                else if (beta == last_score + aspiration_bounds[1]) beta = last_score + aspiration_bounds[2];
                else beta = 20000;
            } 
        }
        if (debug_mode) {
            std::cout << "info string move ordering\ninfo string attempts " << cut_num << " cuts " << beta_cuts << " ratio " << static_cast<double>(cut_num) / beta_cuts << std::endl;
            std::cout << "info string tt\ninfo string queries " << tt_queries << " hits " << tt_hits << " moves " << tt_moves << " cutoffs " << tt_cutoffs << " hit% " << 100.0 * tt_hits / tt_queries << " move% " << 100.0 * tt_moves / tt_queries << " cutoff% " << 100.0 * tt_cutoffs / tt_queries << std::endl;
            std::cout << "info string extensions\ninfo string total nodes " << main_nodes << " extensions " << extended << " ext% " << 100.0 * extended / main_nodes << std::endl;
            std::cout << "info string reductions\ninfo string total nodes " << main_nodes << " reduction attempts " << red_attempts << " reductions " << reduced << " attd% " << 100.0 * red_attempts / main_nodes << " red% " << 100.0 * reduced / main_nodes << " succ% " << 100.0 * reduced / red_attempts << std::endl;
            std::cout << "info string pruning\ninfo string total nodes " << main_nodes << " pruned " << pruned << " prune% " << 100.0 * pruned / (pruned + main_nodes) << std::endl;
            std::cout << "info string null move\ninfo string null move attempts " << null_attempts << " successes " << nulled << " null% " << 100.0 * nulled / null_attempts << std::endl;
        }
        if (output) print_bestmove(out, bestmove);
        return last_score;
    }
}
