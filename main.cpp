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
u64 tthits{0};
Move pv_table[128][128];

std::ifstream infile ("peacekeeper/logs/input.txt");
std::ofstream debug ("logs/debug.txt");
std::ostream& out = std::cout;
std::istream& in = std::cin;

int main() {
    Move move{1, 52, 12, 36};
    Movelist movelist;
    std::string movestring;
    Position position;
    Hashtable hash{1};
    History_table history{};
    Stop_timer timer{0, 0, 0};
    std::atomic<bool>& stop = timer.stop;
    std::string command, token;
    std::vector<std::string> tokens;
    while (true) {
        getline(in, command);
        tokens.clear();
        std::istringstream parser(command);
        while (parser >> token) {
            tokens.push_back(token);
        }
        if (tokens.size() == 0) {
            continue;
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
            if (std::find(tokens.begin(), tokens.end(), "movetime") != tokens.end()) {
                movetime = stoi(*(++std::find(tokens.begin(), tokens.end(), "movetime")));
            }
            if (std::find(tokens.begin(), tokens.end(), "depth") != tokens.end()) {
                depth = stoi(*(++std::find(tokens.begin(), tokens.end(), "depth")));
            }
            if (std::find(tokens.begin(), tokens.end(), "nodes") != tokens.end()) {
                nodes = stoi(*(++std::find(tokens.begin(), tokens.end(), "nodes")));
            }
            if (std::find(tokens.begin(), tokens.end(), "wtime") != tokens.end()) {
                calculate = true;
                wtime = stoi(*(++std::find(tokens.begin(), tokens.end(), "wtime")));
            }
            if (std::find(tokens.begin(), tokens.end(), "btime") != tokens.end()) {
                calculate = true;
                btime = stoi(*(++std::find(tokens.begin(), tokens.end(), "btime")));
            }
            if (std::find(tokens.begin(), tokens.end(), "winc") != tokens.end()) {
                winc = stoi(*(++std::find(tokens.begin(), tokens.end(), "winc")));
            }
            if (std::find(tokens.begin(), tokens.end(), "binc") != tokens.end()) {
                binc = stoi(*(++std::find(tokens.begin(), tokens.end(), "binc")));
            }
            if (std::find(tokens.begin(), tokens.end(), "movestogo") != tokens.end()) {
                movestogo = stoi(*(++std::find(tokens.begin(), tokens.end(), "movestogo")));
            }
            if (movetime == 0 && calculate == true) {
                int mytime{position.side_to_move ? wtime : btime};
                int myinc{position.side_to_move ? winc : binc};
                if (movestogo == 0 || movestogo > std::min(15, (50 - position.ply / 6))) {
                    movestogo = std::min(15, (50 - position.ply / 6)); //estimated number of moves until fresh time or end of game
                }
                movetime = mytime / movestogo + winc; //time usable in terms of time remaining and increment
                movetime = std::min(movetime, mytime); //in case time is low, do not run out of time on this move
                movetime -= 50; //accounting for lag, network delay, etc
                movetime = std::max(0, movetime); //no negative movetime
            }
            timer.reset(movetime, nodes, depth);
            std::thread search{iterative_deepening, std::ref(position), std::ref(timer), std::ref(hash), std::ref(history), std::ref(move)};
            search.detach();
        }
        if (tokens[0] == "isready") {
            out << "readyok" << std::endl;
        }
        if (tokens[0] == "perft") {
            int depth = 1;
            if (std::find(tokens.begin(), tokens.end(), "depth") != tokens.end()) {
                depth = stoi(*(++std::find(tokens.begin(), tokens.end(), "depth")));
            }
            Timer timer;
            timer.reset();
            u64 result = perft(position, depth);
            double end = timer.elapsed();
            out << "info nodes " << result << " time " << static_cast<int>(end * 1000) << " nps " << static_cast<int>(result / end) << std::endl;
        }
        if (tokens[0] == "position") {
            if (tokens.size() >= 2 && tokens[1] == "startpos") {
                position.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1");
            } else if (tokens.size() >= 7) {
                position.load_fen(tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]);
            }
            if (std::find(tokens.begin(), tokens.end(), "moves") != tokens.end()) {
                for (auto iter = ++std::find(tokens.begin(), tokens.end(), "moves"); iter != tokens.end(); ++iter) {
                    position.parse_move(move, *iter);
                    position.make_move(move);
                }
            }
        }
        if (tokens[0] == "quit") {
            stop = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); //delay to ensure that the search stops
            return 0;
        }
        if (tokens[0] == "setoption" && tokens.size() >= 4) {
            if (tokens[1] == "Hash" && tokens[2] == "value") {
                hash.resize(stoi(tokens[3]));
            }
        }
        if (tokens[0] == "stop") {
            stop = true;
        }
        if (tokens[0] == "uci") {
            print_info(out);
        }
        if (tokens[0] == "ucinewgame") {
            hash.clear();
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

int quiescence(Position& position, Stop_timer& timer, int ply, int alpha, int beta) {
    if (!(nodes & 8191) && timer.check(nodes)) return 0;
    if (position.check()) {
        int result = -20000;
        Movelist movelist;
        position.legal_moves(movelist);
        if (movelist.size() == 0) {
            return -20000+ply;
        }
        for (int i = 0; i < movelist.size(); ++i) {
            movelist[i].add_sortkey(movelist[i].evade_order());
        }
        movelist.sort(0, movelist.size());
        for (int i = 0; i < movelist.size(); ++i) {
            position.make_move(movelist[i]);
            ++nodes;
            if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
            result = -quiescence(position, timer, ply + 1, -beta, -alpha);
            if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
            position.undo_move(movelist[i]);
            if (result > alpha) {
                alpha = result;
                if (alpha >= beta) {
                    return alpha;
                }
            }
        }
        return alpha;
    } else {
        int stand_pat = position.static_eval();
        if (stand_pat >= beta) {
            return stand_pat;
        } //if the position is already so good, cutoff immediately
        if (alpha < stand_pat) alpha = stand_pat;
        Movelist movelist;
        position.legal_noisy(movelist);
        for (int i = 0; i < movelist.size(); ++i) {
            movelist[i].add_sortkey(movelist[i].mvv_lva());
        }
        movelist.sort(0, movelist.size());
        int result = -20000;
        for (int i = 0; i < movelist.size(); ++i) {
            position.make_move(movelist[i]);
            ++nodes;
            if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
            result = -quiescence(position, timer, ply + 1, -beta, -alpha);
            if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
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
    if (depth <= 0) return quiescence(position, timer, ply + 1, alpha, beta);
    if (!(nodes & 8191) && timer.check(nodes)) return 0;
    if (position.draw()) return 0;
    if (depth == 1) pv_table[ply + 1][0] = Move{};
    int mating_value = 20000 - ply;
    bool in_check = position.check();//condition for NMP, futility, and LMR
    bool static_eval = position.static_eval();
    int moves_searched{0};
    int result{alpha}; //initialized to alpha just in case futility pruning prunes all the nodes and we have to return something
    int old_alpha{alpha};
    int reduction{};
    Move bestmove{};
    Movelist movelist;//finally generating and sorting moves
    position.legal_noisy(movelist);
    for (int i = 0; i < movelist.size(); ++i) {
        movelist[i].add_sortkey(movelist[i].mvv_lva());
    }
    movelist.sort(0, movelist.size());
    for (int i{}; i < movelist.size(); ++i) {
        position.make_move(movelist[i]);
        ++nodes;
        if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
        if (depth == 1 || !is_pv || moves_searched == 0) {
            result = -pvs(position, timer, table, history, depth - 1, ply + 1, -beta, -alpha, is_pv, true);
        } else {
            result = -pvs(position, timer, table, history, depth - 1, ply + 1, -alpha-1, -alpha, false, true);
            if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
            if (alpha < result && result < beta) {
                result = -pvs(position, timer, table, history, depth - 1, ply + 1, -beta, -alpha, is_pv, true);
            }
        }
        if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
        position.undo_move(movelist[i]);
        ++moves_searched;
        
        if (result > alpha) {
            alpha = result;
            bestmove = movelist[i];
            if (is_pv) {
                pv_table[ply][0] = bestmove;
                memcpy(&pv_table[ply][1], &pv_table[ply+1][0], sizeof(Move) * 127);
            }
        }
    }
    position.legal_quiet(movelist);
    for (int i = 0; i < movelist.size(); ++i) {
        movelist[i].add_sortkey(history.table[movelist[i].piece()][movelist[i].end()] + movelist[i].quiet_order());
    }
    movelist.sort(0, movelist.size());
    for (int i{}; i < movelist.size(); ++i) {
        position.make_move(movelist[i]);
        ++nodes;
        if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
        if (depth == 1 || !is_pv || moves_searched == 0) {
            result = -pvs(position, timer, table, history, depth - 1, ply + 1, -beta, -alpha, is_pv, true);
        } else {
            result = -pvs(position, timer, table, history, depth - 1, ply + 1, -alpha-1, -alpha, false, true);
            if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
            if (alpha < result && result < beta) {
                result = -pvs(position, timer, table, history, depth - 1, ply + 1, -beta, -alpha, is_pv, true);
            }
        }
        if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); return 0;}
        position.undo_move(movelist[i]);
        ++moves_searched;
        if (result > alpha) {
            alpha = result;
            bestmove = movelist[i];
            if (is_pv) {
                pv_table[ply][0] = bestmove;
                memcpy(&pv_table[ply][1], &pv_table[ply+1][0], sizeof(Move) * 127);
            }
            if (alpha >= beta) {
                for (int j{0}; j<i; ++j) {
                    history.edit(movelist[j].piece(), movelist[j].end(), -depth);
                }
                history.edit(bestmove.piece(), bestmove.end(), depth * depth);
                table.insert(position.hashkey(), alpha, tt_beta, bestmove, depth);
                return alpha;
            }
        }
    }
    if (moves_searched + fut_pruned == 0) {//checking for checkmate, stalemate
        if (in_check)
            return -20000 + ply;
        else
            return 0;
    }
    table.insert(position.hashkey(), alpha, ((alpha > old_alpha)?tt_exact:tt_alpha), bestmove, depth);
    return alpha;
}

void iterative_deepening(Position& position, Stop_timer& timer, Hashtable& table, History_table& history, Move& bestmove) {
    history.age();
    Movelist movelist;
    position.legal_moves(movelist);
    if (movelist.size() == 0) {
        return;
    } else {
        int alpha = -20000;
        nodes = 0;
        tthits = 0;
        Element entry = table.query(position.hashkey());
        for (int i = 0; i < movelist.size(); ++i) {
            movelist[i].add_sortkey(history.table[movelist[i].piece()][movelist[i].end()] + movelist[i].order());
        }
        if (entry.type != nonexistent && entry.full_hash == position.hashkey() && entry.bestmove.not_null()) {
            for (int i{0}; i < movelist.size(); ++i) {
                if (movelist[i] == entry.bestmove) {
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
            if (timer.check(nodes, depth)) {break;}
            last_score = alpha;
            alpha = -20000;
            for (int i{0}; i < movelist.size(); ++i) {
                position.make_move(movelist[i]);
                ++nodes;
                if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); break;}
                orig_nodes = nodes;
                result = -pvs(position, timer, table, history, depth - 1, 1, -alpha-1, -alpha, false, true);
                if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); break;}
                if (depth > 1 && alpha < result) {
                    result = -pvs(position, timer, table, history, depth-1, 1, -20000, -alpha, true, true);
                    if (!(nodes & 8191) && timer.check(nodes)) {position.undo_move(movelist[i]); break;}
                }
                movelist[i].add_sortkey(nodes - orig_nodes);
                position.undo_move(movelist[i]);
                if (result > alpha) {
                    alpha = result;
                    bestmove = movelist[i];
                    pv_table[0][0] = bestmove;
                    memcpy(&pv_table[0][1], &pv_table[1][0], sizeof(Move) * 127);
                }
            }
            if (!(nodes & 8191) && timer.check(nodes)) break;
            if (movelist[0] == bestmove) {
                print_uci(out, alpha == -20000 ? last_score : alpha, depth, nodes, static_cast<int>(nodes/timer.elapsed()), static_cast<int>(timer.elapsed()*1000), pv_table[0]);
                movelist.sort(1, movelist.size());
                continue;
            }
            for (int i{0}; i < movelist.size(); ++i) {
                if (movelist[i] == bestmove) {
                    movelist[i] = movelist[0];
                    movelist[0] = bestmove;
                }
            }
            movelist.sort(1, movelist.size());
            print_uci(out, alpha == -20000 ? last_score : alpha, depth, nodes, static_cast<int>(nodes/timer.elapsed()), static_cast<int>(timer.elapsed()*1000), pv_table[0]);
        }
        table.insert(position.hashkey(), alpha, tt_exact, bestmove, depth);
        print_bestmove(out, bestmove);
        return;
    }
}