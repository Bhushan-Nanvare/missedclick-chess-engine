#include "search/search.hpp"
#include "board/board.hpp"
#include "moves/makemove.hpp"
#include "moves/movegen.hpp"
#include "search/moveorder.hpp"
#include "search/tt.hpp"
#include "eval/evaluate.hpp"
#include "utils/utils.hpp"
#include "engine/uci.hpp"
#include "board/hash.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace MissedClick {

// Core search: negamax + alpha-beta, TT, null move, LMR, quiescence. Scores are always
// from the side-to-move viewpoint at the node; children are searched with negated window.

static constexpr int INF = 50000;

SearchInfo search_info;

// How often to check for time (every N nodes)
static const int COMMUNICATE_EVERY = 2047;

void search_position() {
    search_info.nodes     = 0;
    search_info.fh        = 0;
    search_info.fhf       = 0;
    search_info.stopped   = 0;
    search_info.best_move = 0;          // will be set live inside negamax

    clear_move_ordering();
    iterative_deepening();
}

int negamax(int alpha, int beta, int depth) {
    if ((search_info.nodes & COMMUNICATE_EVERY) == 0)
        communicate();
    if (search_info.stopped)
        return 0;

    search_info.nodes++;

    // Check for draw by repetition or fifty-move rule (not at root)
    if (ply > 0 && is_repetition()) return 0;
    if (ply > 0 && fifty >= 100) return 0;

    // Static eval is too noisy on tactical positions; quiescence searches captures only.
    if (depth == 0)
        return quiescence(alpha, beta);

    // Shrink the window toward known mate bounds so we don't chase impossible scores.
    int mate_value = MATE_VALUE - ply;
    if (alpha < -mate_value) alpha = -mate_value;
    if (beta  >  mate_value) beta  =  mate_value;
    if (alpha >= beta) return alpha;

    int tt_score = 0, tt_move = 0;
    if (read_tt(hash_key, depth, alpha, beta, &tt_score, &tt_move))
        return tt_score;

    bool in_check = is_square_attacked(
        (side == white) ? get_ls1b_index(bitboards[K]) : get_ls1b_index(bitboards[k]),
        side ^ 1);

    int effective_depth = depth;
    if (in_check && depth >= 1)
        effective_depth++;

    // Null move: give opponent two plies in a row; if still refuted, position is probably safe to prune.
    if (!in_check && depth >= 3 && ply > 0) {
        // copy_board() saves at current ply; undo_move() will restore it after ply--
        copy_board();

        // Make null move manually (no piece movement, just flip side & clear ep)
        if (enpassant != no_sq) hash_key ^= enpassant_keys[enpassant];
        enpassant = no_sq;
        side ^= 1;
        hash_key ^= side_key;
        ply++;

        int null_score = -negamax(-beta, -beta + 1, depth - 3);

        // Undo null move: undo_move() decrements ply and fully restores all board state
        undo_move();

        if (search_info.stopped) return 0;
        if (null_score >= beta)
            return beta;
    }

    MoveList move_list;
    generate_moves(&move_list);
    sort_moves(&move_list);

    int legal_moves = 0;
    int old_alpha = alpha;

    for (int count = 0; count < move_list.count; count++) {
        int move = move_list.moves[count];

        // Save current hash BEFORE making move (repetition tracks visited positions)
        repetition_table[repetition_index++] = hash_key;

        if (!make_move(move, all_moves)) {
            repetition_index--;
            continue;
        }

        legal_moves++;
        ply++;

        // LMR: likely-quiet later moves are searched one ply shallower (ordering matters a lot).
        int new_depth = effective_depth - 1;
        if (count >= FULL_DEPTH_MOVES && depth >= REDUCTION_LIMIT &&
            !get_move_capture(move) && !get_move_promoted(move) && !in_check) {
            new_depth--;
        }

        int score = -negamax(-beta, -alpha, new_depth);

        // undo_move() decrements ply internally
        undo_move();
        repetition_index--;

        if (search_info.stopped) return 0;

        if (score >= beta) {
            write_tt(hash_key, depth, TT_BETA, beta, move);
            if (!get_move_capture(move)) {
                update_killer_moves(ply, move);
                update_history_scores(move, depth);
            }
            search_info.fh++;
            if (legal_moves == 1) search_info.fhf++;
            return beta;
        }

        if (score > alpha) {
            write_tt(hash_key, depth, TT_EXACT, score, move);
            alpha = score;
            // Track root best move directly — bypasses TT-collision risk
            if (ply == 0) search_info.best_move = move;
        }
    }

    if (legal_moves == 0) {
        if (in_check)
            return -MATE_VALUE + ply; // shorter mates preferred (ply increases toward root)
        return 0;
    }

    if (alpha == old_alpha)
        write_tt(hash_key, depth, TT_ALPHA, alpha, 0);

    return alpha;
}

int quiescence(int alpha, int beta) {
    if ((search_info.nodes & COMMUNICATE_EVERY) == 0)
        communicate();
    if (search_info.stopped)
        return 0;

    search_info.nodes++;

    if (ply > 0 && is_repetition()) return 0;
    if (fifty >= 100) return 0;

    int eval = evaluate(); // "stand pat": assume we can stop capturing; still search captures that improve.

    if (eval >= beta) return beta;
    if (eval > alpha) alpha = eval;

    MoveList move_list;
    generate_moves(&move_list);
    sort_moves(&move_list);

    for (int count = 0; count < move_list.count; count++) {
        int move = move_list.moves[count];

        if (!get_move_capture(move))
            continue;

        repetition_table[repetition_index++] = hash_key;

        if (!make_move(move, all_moves)) {
            repetition_index--;
            continue;
        }

        ply++;

        int score = -quiescence(-beta, -alpha);

        undo_move();
        repetition_index--;

        if (search_info.stopped) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int pvs(int alpha, int beta, int depth) {
    return negamax(alpha, beta, depth);
}

int lmr(int depth, int moves_searched) {
    if (moves_searched > 4 && depth > 2)
        return depth - 1;
    return depth;
}

int nmp(int beta, int depth) {
    return negamax(beta - 1, beta, depth - 3);
}

void iterative_deepening() {
    int game_rep_index = repetition_index;
    U64 root_hash      = hash_key;

    // Each iteration rewinds to this root; fallback_move avoids sending an empty bestmove.
    int fallback_move = 0;
    {
        MoveList ml; generate_moves(&ml);
        if (ml.count > 0) fallback_move = ml.moves[0];
    }

    int best_score = 0;

    for (int current_depth = 1; current_depth <= search_info.depth_limit; current_depth++) {
        if ((current_depth & 7) == 0) communicate();
        if (search_info.stopped) break;

        ply              = 0;
        repetition_index = game_rep_index;
        hash_key         = root_hash;

        int score = negamax(-INF, INF, current_depth);

        if (search_info.stopped) break;

        best_score = score;

        // Cross-check: TT lookup immediately after negamax (TT still hot for root)
        // search_info.best_move was already updated inside negamax at ply==0.
        // Use TT as secondary confirmation only; prefer the live-tracked value.
        {
            ply      = 0;
            hash_key = root_hash;
            int tt_best = get_pv_move();
            if (tt_best != 0) search_info.best_move = tt_best;
        }

        // Guarantee we always have a move
        if (search_info.best_move == 0)
            search_info.best_move = fallback_move;

        // Build PV for info output
        int pv_moves[MAX_PLY];
        int pv_count = 0;
        {
            ply = 0;
            repetition_index = game_rep_index;
            hash_key = root_hash;

            for (int d = 0; d < current_depth && d < MAX_PLY; d++) {
                int pv_move = get_pv_move();
                if (pv_move == 0) break;
                if (!make_move(pv_move, all_moves)) break;
                repetition_table[repetition_index++] = hash_key;
                ply++;
                pv_moves[pv_count++] = pv_move;
            }

            // Undo all PV moves
            for (int i = 0; i < pv_count; i++) {
                repetition_index--;
                undo_move();
            }

            // Restore root state
            ply = 0;
            repetition_index = game_rep_index;
            hash_key = root_hash;
        }

        // Output UCI info
        int elapsed = get_time_ms() - search_info.starttime;
        if (elapsed < 1) elapsed = 1;
        long long nps = (long long)search_info.nodes * 1000 / elapsed;

        if (best_score > MATE_SCORE) {
            int mate_in = (MATE_VALUE - best_score + 1) / 2;
            printf("info depth %d score mate %d nodes %d nps %lld time %d pv",
                   current_depth, mate_in, search_info.nodes, nps, elapsed);
        } else if (best_score < -MATE_SCORE) {
            int mate_in = (MATE_VALUE + best_score + 1) / 2;
            printf("info depth %d score mate -%d nodes %d nps %lld time %d pv",
                   current_depth, mate_in, search_info.nodes, nps, elapsed);
        } else {
            printf("info depth %d score cp %d nodes %d nps %lld time %d pv",
                   current_depth, best_score, search_info.nodes, nps, elapsed);
        }

        for (int i = 0; i < pv_count; i++) {
            printf(" ");
            print_move(pv_moves[i]);
        }
        printf("\n");
        fflush(stdout);

        if (abs(best_score) > MATE_SCORE)
            break;
    }

    // Restore root state before returning
    ply = 0;
    repetition_index = game_rep_index;
    hash_key = root_hash;
}

void enable_time_management(int time_ms) {
    search_info.starttime = get_time_ms();
    search_info.stoptime = search_info.starttime + time_ms;
    search_info.timeset = 1;
}

bool is_time_up() {
    if (!search_info.timeset)
        return false;
    if (get_time_ms() >= search_info.stoptime) {
        search_info.stopped = 1;
        return true;
    }
    return false;
}

void stop_search() {
    search_info.stopped = 1;
}

void print_thinking(int depth, int score, int nodes, int time, MoveList* pv) {
    int nps = (time > 0) ? (int)((long long)nodes * 1000 / time) : 0;
    printf("info depth %d score cp %d nodes %d nps %d time %d pv",
           depth, score, nodes, nps, time);
    for (int i = 0; i < pv->count; i++) {
        printf(" ");
        print_move(pv->moves[i]);
    }
    printf("\n");
    fflush(stdout);
}

void get_pv_line(int depth, MoveList* pv) {
    pv->count = 0;
    for (int i = 0; i < depth && i < MAX_PLY; i++) {
        int move = get_pv_move();
        if (move == 0) break;
        if (!make_move(move, all_moves)) break;
        repetition_table[repetition_index++] = hash_key;
        ply++;
        pv->moves[pv->count++] = move;
    }
    for (int i = 0; i < pv->count; i++) {
        repetition_index--;
        undo_move();
    }
}

bool is_repetition() {
    for (int i = 0; i < repetition_index; i++) {
        if (repetition_table[i] == hash_key)
            return true;
    }
    return false;
}

void update_search_stats(int move, int depth) {
    update_history_scores(move, depth);
    update_killer_moves(ply, move);
}

} // namespace MissedClick
