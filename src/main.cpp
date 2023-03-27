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

int main() {
    Move move{};
    Movelist movelist;
    std::string movestring;
    Position position;
    Hashtable hash{1};
    History_table history{};
    Stop_timer timer{0, 0, 0};
    int move_overhead = 20;
    std::atomic<bool>& stop = timer.stop;
    std::string command, token;
    std::vector<std::string> tokens;
    std::cout << "peacekeeper by sazgr" << std::endl;
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
        if (tokens[0] == "eval") {
            out << position << "PSQT: " << position.static_eval() << std::endl;
        }
        if (tokens[0] == "go") {
            if (std::find(tokens.begin(), tokens.end(), "infinite") != tokens.end()) {
                timer.reset();
                std::thread search{iterative_deepening, std::ref(position), std::ref(timer), std::ref(hash), std::ref(history), std::ref(move)};
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
            if (movetime == 0 && calculate == true) {
                int mytime{position.side_to_move ? wtime : btime};
                int myinc{position.side_to_move ? winc : binc};
                if (movestogo == 0 || movestogo > std::max(30, (50 - position.ply / 5))) {movestogo = std::max(30, (50 - position.ply / 5));} //estimated number of moves until fresh time or end of game
                movetime = mytime / movestogo + winc; //time usable in terms of time remaining and increment
                movetime = std::min(movetime, mytime); //in case time is low, do not run out of time on this move
                movetime -= move_overhead; //accounting for lag, network delay, etc
                movetime = std::max(1, movetime); //no negative movetime
            }
            timer.reset(movetime, nodes, depth);
            std::thread search{iterative_deepening, std::ref(position), std::ref(timer), std::ref(hash), std::ref(history), std::ref(move)};
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
        if (tokens[0] == "quiesce") {
            timer.reset(0, 0, 0);
            quiescence(position, timer, 0, -20000, 20000);
            return 0;
        }
        if (tokens[0] == "quit") {
            stop = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); //wait 200 milliseconds to make sure that any ongoing searches stop before quitting
            return 0;
        }
        if (tokens[0] == "search") {
            timer.reset(0, 0, 0);
            iterative_deepening(position, timer, hash, history, move);
        }
        if (tokens[0] == "setoption" && tokens[1] == "name") {
            if (tokens.size() >= 5 && tokens[2] == "Hash" && tokens[3] == "value") {hash.resize(stoi(tokens[4]));}
            if (tokens.size() >= 6 && tokens[2] == "Move" && tokens[3] == "Overhead" && tokens[4] == "value") {move_overhead = stoi(tokens[5]);}
        }
        if (tokens[0] == "stop") {stop = true;}
        if (tokens[0] == "uci") {print_info(out);}
        if (tokens[0] == "ucinewgame") {
            hash.clear();
            history.reset();
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

int quiescence(Position& position, Stop_timer& timer, int ply, int alpha, int beta) {
    if (timer.stopped() || (!(nodes & 8191) && timer.check(nodes))) return 0;
    if (position.check()) {
        int result = -20000;
        Movelist movelist;
        position.legal_moves(movelist);
        if (movelist.size() == 0) return -20000 + ply;
        for (int i = 0; i < movelist.size(); ++i) movelist[i].add_sortkey(movelist[i].evade_order());
        movelist.sort(0, movelist.size());
        for (int i = 0; i < movelist.size(); ++i) {
            position.make_move(movelist[i]);
            ++nodes;
            result = -quiescence(position, timer, ply + 1, -beta, -alpha);
            position.undo_move(movelist[i]);
            if (result > alpha) {
                alpha = result;
                if (alpha >= beta) return alpha;
            }
        }
        return alpha;
    } else {
        int static_eval = position.static_eval();
        if (static_eval >= beta) return static_eval; //if the position is already so good, cutoff immediately
        if (alpha < static_eval) alpha = static_eval;
        Movelist movelist;
        position.legal_noisy(movelist);
        for (int i = 0; i < movelist.size(); ++i) movelist[i].add_sortkey(movelist[i].mvv_lva());
        movelist.sort(0, movelist.size());
        int result = -20000;
        for (int i = 0; i < movelist.size(); ++i) {
            if constexpr (delta_pruning) if (static_eval + movelist[i].gain() + futile_margins[0] < alpha) break; //delta pruning
            position.make_move(movelist[i]);
            ++nodes;
            result = -quiescence(position, timer, ply + 1, -beta, -alpha);
            position.undo_move(movelist[i]);
            if (result > alpha) {
                alpha = result;
                if (alpha >= beta) {
                    return alpha;
                }
            }
        }
        return alpha;
    }
}

int pvs(Position& position, Stop_timer& timer, Hashtable& table, History_table& history, int depth, int ply, int alpha, int beta, bool is_pv, bool can_null)
{
    if (timer.stopped() || (!(nodes & 8191) && timer.check(nodes))) return 0;
    if (depth <= 0) return quiescence(position, timer, ply + 1, alpha, beta);
    if (depth == 1 && is_pv) pv_table[ply + 1][0] = Move{};
    if (position.draw(ply > 2 ? 1 : 2)) {
        pv_table[ply][0] = Move{};
        return 0;
    }
    bool in_check = position.check();//condition for NMP, futility, and LMR
    int static_eval = position.static_eval();
    int move_num{0};
    int result{};
    int old_alpha{alpha};
    int reduce_all{1};
    int reduce_this{};
    Move bestmove{};
    if constexpr (static_null_move) if (depth < 4 && !is_pv && !in_check && no_mate(alpha, beta) && (static_eval - futile_margins[depth] >= beta)) {
        ++pruned;
        return static_eval - futile_margins[depth];
    }
    if constexpr (null_move_pruning) if (depth > 1 && can_null && !is_pv && static_eval > beta && (position.eval_phase() > 4) && !in_check) {
        position.make_null();
        ++nodes;
        ++null_attempts;
        result = -pvs(position, timer, table, history, std::max(1, depth - reduce_all - 3 - (depth > 7) - (depth > 11) - (depth > 16)), ply + 1, -beta, -beta + 1, false, false);
        position.undo_null();
        if (!timer.stopped() && result >= beta) {
            //if (!timer.stopped) table.insert(position.hashkey(), result, tt_beta, bestmove, depth);
            ++nulled;
            return result;
        }
    }
    Element entry = table.query(position.hashkey());
    Move hash_move = Move(entry.bestmove);
    ++tt_queries;
    if (entry.type != tt_none && entry.full_hash == position.hashkey()) ++tt_hits;
    if (!is_pv && entry.type != tt_none && entry.full_hash == position.hashkey() && entry.depth >= depth && no_mate(entry.score, entry.score) && (entry.type == tt_exact || (entry.type == tt_alpha && entry.score <= alpha) || (entry.type == tt_beta && entry.score >= beta))) {
        ++tt_cutoffs;
        return entry.score;
    }
    if constexpr (check_extensions) if (in_check && depth <= 2) {++extended; --reduce_all;} //check extension
    bool hash_move_usable = entry.type != tt_none && entry.full_hash == position.hashkey() && hash_move.not_null() && position.board[hash_move.start()] == hash_move.piece() && position.board[hash_move.end()] == hash_move.captured();
    //Stage 1 - Hash Move
    if (hash_move_usable) {//searching best move from hashtable
        position.make_move(hash_move);
        ++nodes;
        ++move_num;
        ++main_nodes;
        ++tt_moves;
        result = -pvs(position, timer, table, history, depth - reduce_all, ply + 1, -beta, -alpha, is_pv, true);
        position.undo_move(hash_move);
        if (!timer.stopped() && result > alpha) {
            alpha = result;
            bestmove = hash_move;
            if (is_pv) {
                pv_table[ply][0] = bestmove;
                memcpy(&pv_table[ply][1], &pv_table[ply+1][0], sizeof(Move) * 127);
            }
            if (alpha >= beta) {
                if constexpr (history_heuristic) if (bestmove.captured() == 12) history.edit(bestmove.piece(), bestmove.end(), depth * depth);
                table.insert(position.hashkey(), alpha, tt_beta, bestmove, depth);
                ++beta_cuts;
                cut_num += move_num;
                return alpha;
            }
        }
    }
    Movelist movelist;
    position.legal_noisy(movelist);
    for (int i = 0; i < movelist.size(); ++i) movelist[i].add_sortkey(movelist[i].mvv_lva());
    movelist.sort(0, movelist.size());
    bool can_fut_prune = !in_check && no_mate(alpha, beta) && depth < 4;
    //Stage 2 - Captures
    for (int i{}; i < movelist.size(); ++i) {
        if (hash_move_usable && movelist[i] == hash_move) continue; //continuing if we already searched the hash move
        position.make_move(movelist[i]);
        ++nodes;
        ++move_num;
        ++main_nodes;
        if (depth == 1 || !is_pv || move_num == 1) {
            result = -pvs(position, timer, table, history, depth - reduce_all, ply + 1, -beta, -alpha, is_pv, true);
        } else {
            result = -pvs(position, timer, table, history, depth - reduce_all, ply + 1, -alpha-1, -alpha, false, true);
            if (alpha < result && result < beta) {
                result = -pvs(position, timer, table, history, depth - reduce_all, ply + 1, -beta, -alpha, is_pv, true);
            }
        }
        position.undo_move(movelist[i]);
        if (!timer.stopped() && result > alpha) {
            alpha = result;
            bestmove = movelist[i];
            if (is_pv) {
                pv_table[ply][0] = bestmove;
                memcpy(&pv_table[ply][1], &pv_table[ply+1][0], sizeof(Move) * 127);
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
        if constexpr (history_heuristic) movelist[i].add_sortkey(history.table[movelist[i].piece()][movelist[i].end()]);
    }
    movelist.sort(0, movelist.size());
    //Stage 3 - Quiet Moves
    for (int i{}; i < movelist.size(); ++i) {
        if (hash_move_usable && movelist[i] == hash_move) continue; //continuing if we already searched the hash move
        position.make_move(movelist[i]);
        bool gives_check = position.check();
        //Futility Pruning
        if constexpr (futility_pruning) if (can_fut_prune && (static_eval + futile_margins[depth] - late_move_margin(depth, move_num) < alpha) && move_num != 0 && !gives_check) {
            position.undo_move(movelist[i]);
            ++pruned;
            continue;
        }
        ++nodes;
        ++move_num;
        ++main_nodes;
        //Late Move Reductions
        reduce_this = 0;
        if constexpr (late_move_reductions) if (depth > 2 && move_num >= 4 && !in_check && history.table[movelist[i].piece()][movelist[i].end()] <= (history.sum >> 10) && !gives_check) {
            reduce_this = lmr_reduction(is_pv, depth, move_num);
        }
        if (move_num == 1) {
            result = -pvs(position, timer, table, history, depth - reduce_all, ply + 1, -beta, -alpha, is_pv, true);
        } else {
            if (reduce_this) { //try a reduced search
                ++red_attempts;
                result = -pvs(position, timer, table, history, depth - reduce_all - reduce_this, ply + 1, -alpha - 1, -alpha, false, true);
                if (result <= alpha || (result >= beta && is_pv)) ++reduced;
            }
            if (!reduce_this || (alpha < result && (result < beta || !is_pv))) {
                result = -pvs(position, timer, table, history, depth - reduce_all, ply + 1, -alpha - 1, -alpha, false, true);
            }
            if (alpha < result && result < beta && is_pv) {
                result = -pvs(position, timer, table, history, depth - reduce_all, ply + 1, -beta, -alpha, is_pv, true);
            }
        }
        position.undo_move(movelist[i]);
        if (!timer.stopped() && result > alpha) {
            alpha = result;
            bestmove = movelist[i];
            if (is_pv) {
                pv_table[ply][0] = bestmove;
                memcpy(&pv_table[ply][1], &pv_table[ply+1][0], sizeof(Move) * 127);
            }
            if (alpha >= beta) {
                if constexpr (history_heuristic) for (int j{0}; j<i; ++j) history.edit(movelist[j].piece(), movelist[j].end(), -depth);
                if constexpr (history_heuristic) history.edit(bestmove.piece(), bestmove.end(), depth * depth);
                table.insert(position.hashkey(), alpha, tt_beta, bestmove, depth);
                ++beta_cuts;
                cut_num += move_num;
                return alpha;
            }
        }
    }
    if (move_num == 0) {
        pv_table[ply][0] = Move{};
        if (in_check) return -20000 + ply;
        else return 0;
    }
    if (!timer.stopped()) table.insert(position.hashkey(), alpha, ((alpha > old_alpha)?tt_exact:tt_alpha), bestmove, depth);
    return timer.stopped() ? 0 : alpha;
}

void iterative_deepening(Position& position, Stop_timer& timer, Hashtable& table, History_table& history, Move& bestmove) {
    if constexpr (history_heuristic) history.age();
    table.age();
    Movelist movelist;
    position.legal_moves(movelist);
    if (movelist.size() == 0) return;
    else {
        int alpha = -20000;
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
        Element entry = table.query(position.hashkey());
        Move hash_move = Move(entry.bestmove);
        if constexpr (history_heuristic) for (int i = 0; i < movelist.size(); ++i) {
            movelist[i].add_sortkey(history.table[movelist[i].piece()][movelist[i].end()]);
        }
        if (entry.type != tt_none && entry.full_hash == position.hashkey() && hash_move.not_null()) {
            for (int i{0}; i < movelist.size(); ++i) {
                if (movelist[i] == hash_move) {
                    Move temp = movelist[0];
                    movelist[0] = movelist[i];
                    movelist[i] = temp;
                    break;
                }
            }
            movelist.sort(1, movelist.size());//sorting on history score, tiebreaking using PSTs
        } else {
            movelist.sort(0, movelist.size());
        }
        int depth{1};
        int result{0};
        int orig_nodes;
        int last_score;
        pv_table[1][0] = Move{};
        bestmove = movelist[0];//make sure we return a legal move
        for (; depth <= 64; ++depth) {
            if (movelist.size() == 0) return;
            if (timer.percent_time() >= 80) {break;}
            if (movelist.size() == 1 && timer.percent_time() >= 40) {break;} //if there is only one move to make do not use as much time
            if (timer.check(nodes, depth)) {break;}
            last_score = alpha;
            alpha = -20000;
            for (int i{0}; i < movelist.size(); ++i) {
                position.make_move(movelist[i]);
                ++nodes;
                orig_nodes = nodes;
                if (depth == 1 || i == 0) {
                    result = -pvs(position, timer, table, history, depth - 1, 1, -20000, -alpha, true, true);
                } else {
                    result = -pvs(position, timer, table, history, depth - 1, 1, -alpha-1, -alpha, false, true);
                    if (result > alpha) {
                        result = -pvs(position, timer, table, history, depth - 1, 1, -20000, -alpha, true, true);
                    }
                }
                movelist[i].add_sortkey(nodes - orig_nodes);
                position.undo_move(movelist[i]);
                if (!timer.stopped() && result > alpha) {
                    alpha = result;
                    bestmove = movelist[i];
                    pv_table[0][0] = bestmove;
                    memcpy(&pv_table[0][1], &pv_table[1][0], sizeof(Move) * 127);
                }
                
            }
            if (movelist[0] != bestmove) {
                for (int i{0}; i < movelist.size(); ++i) {
                    if (movelist[i] == bestmove) {
                        movelist[i] = movelist[0];
                        movelist[0] = bestmove;
                    }
                }
            }
            movelist.sort(1, movelist.size());
            print_uci(out, alpha == -20000 ? last_score : alpha, depth, nodes, static_cast<int>(nodes/timer.elapsed()), static_cast<int>(timer.elapsed()*1000), pv_table[0]);
        }
        table.insert(position.hashkey(), alpha, tt_exact, bestmove, depth);
        if (debug_mode) {
            std::cout << "info string move ordering\ninfo string attempts " << cut_num << " cuts " << beta_cuts << " ratio " << static_cast<double>(cut_num) / beta_cuts << std::endl;
            std::cout << "info string tt\ninfo string queries " << tt_queries << " hits " << tt_hits << " moves " << tt_moves << " cutoffs " << tt_cutoffs << " hit% " << 100.0 * tt_hits / tt_queries << " move% " << 100.0 * tt_moves / tt_queries << " cutoff% " << 100.0 * tt_cutoffs / tt_queries << std::endl;
            std::cout << "info string extensions\ninfo string total nodes " << main_nodes << " extensions " << extended << " ext% " << 100.0 * extended / main_nodes << std::endl;
            std::cout << "info string reductions\ninfo string total nodes " << main_nodes << " reduction attempts " << red_attempts << " reductions " << reduced << " attd% " << 100.0 * red_attempts / main_nodes << " red% " << 100.0 * reduced / main_nodes << " succ% " << 100.0 * reduced / red_attempts << std::endl;
            std::cout << "info string pruning\ninfo string total nodes " << main_nodes << " pruned " << pruned << " prune% " << 100.0 * pruned / (pruned + main_nodes) << std::endl;
            std::cout << "info string null move\ninfo string null move attempts " << null_attempts << " successes " << nulled << " null% " << 100.0 * nulled / null_attempts << std::endl;
        }
        print_bestmove(out, bestmove);
        return;
    }
}
