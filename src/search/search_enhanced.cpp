#include "search/search_enhanced.hpp"
#include "board/board.hpp"
#include "board/bitboard.hpp"
#include "moves/makemove.hpp"
#include "moves/movegen.hpp"
#include "search/moveorder.hpp"
#include "search/tt.hpp"
#include "eval/evaluate.hpp"
#include "utils/utils.hpp"
#include "engine/uci.hpp"
#include "board/hash.hpp"
#include "board/bitboard_optimized.hpp"
#include <cstdio>
#include <cstdlib>

namespace MissedClick {

// Global enhanced search instance
SearchEnhanced g_search_enhanced;
extern SearchInfo search_info;

// Enhanced move-ordering state
int counter_moves[600] = {0};
bool in_check = false;

// Reset statistics
void SearchEnhanced::reset_stats() {
    stats = {0, 0, 0, 0, 0, 0};
}

// Enhanced move ordering with additional heuristics
void SearchEnhanced::score_moves_enhanced(MoveList* move_list, int depth) {
    for (int i = 0; i < move_list->count; i++) {
        int move = move_list->moves[i];
        int score = 0;
        
        // PV move gets highest score
        int pv_move = get_pv_move();
        if (move == pv_move) {
            score += 1000000;
        }
        
        // Capture scoring (MVV-LVA)
        // IMPORTANT: get_move_capture() returns a FLAG (0x100000), not a piece index!
        if (get_move_capture(move)) {
            int moving_piece = get_move_piece(move);
            int target_sq    = get_move_target(move);
            
            // Find actual captured piece from board state
            int victim = 0;
            int start_piece = (side == white) ? p : P;
            int end_piece   = (side == white) ? k : K;
            for (int pp = start_piece; pp <= end_piece; pp++) {
                if (GET_BIT(bitboards[pp], target_sq)) {
                    victim = pp;
                    break;
                }
            }
            // MVV-LVA: attacker row, victim column
            score += mvv_lva_scores[moving_piece][victim] + 1000000;
        }
        
        // Promotion scoring
        if (get_move_promoted(move)) {
            score += 900000;
        }
        
        // Killer moves
        if (move == killer_moves[0][ply]) {
            score += 800000;
        } else if (move == killer_moves[1][ply]) {
            score += 700000;
        }
        
        // History heuristic
        int piece = get_move_piece(move);
        int target_square = get_move_target(move);
        score += history_scores[piece][target_square];
        
        // Counter move bonus
        if (depth > 0 && move == counter_moves[ply]) {
            score += 600000;
        }
        
        // Castling bonus
        if (get_move_castling(move)) {
            score += 500000;
        }
        
        // Check evasion bonus
        if (in_check) {
            score += 400000;
        }
        
        // Passed pawn bonus
        if (is_passed_pawn_move(move)) {
            score += 300000;
        }
        
        move_list->scores[i] = score;
    }
    
    // Sort moves using insertion sort (efficient for partially sorted data)
    for (int current = 1; current < move_list->count; current++) {
        int temp_move = move_list->moves[current];
        int temp_score = move_list->scores[current];
        int i = current - 1;
        
        while (i >= 0 && move_list->scores[i] < temp_score) {
            move_list->moves[i + 1] = move_list->moves[i];
            move_list->scores[i + 1] = move_list->scores[i];
            i--;
        }
        
        move_list->moves[i + 1] = temp_move;
        move_list->scores[i + 1] = temp_score;
    }
}

// Late Move Reduction calculation
int SearchEnhanced::get_lmr_reduction(int depth, int move_count, bool in_check) {
    // Don't reduce in check or at very low depth
    if (in_check || depth < LMR_DEPTH_LIMIT) return 0;
    
    // Don't reduce for early moves
    if (move_count < LMR_MOVES_LIMIT) return 0;
    
    // Base reduction
    int reduction = 1;
    
    // More reduction for deeper searches
    if (depth > 6) reduction++;
    
    // More reduction for later moves
    if (move_count > 10) reduction++;
    if (move_count > 20) reduction++;
    
    return reduction;
}

// Check if position is in check
bool SearchEnhanced::is_in_check() {
    return is_square_attacked((side == white) ? get_ls1b_index(bitboards[K]) : get_ls1b_index(bitboards[k]), side ^ 1);
}

// Check if move creates check
bool SearchEnhanced::gives_check(int move) {
    // Make move temporarily
    if (!make_move(move, all_moves)) return false;
    
    bool check = is_in_check();
    
    restore_illegal_move();
    return check;
}

// Check if move is passed pawn
bool SearchEnhanced::is_passed_pawn_move(int move) {
    int piece = get_move_piece(move);
    if (piece != P && piece != p) return false;
    
    int target_square = get_move_target(move);
    int rank = (piece == P) ? (target_square / 8) : (7 - target_square / 8);
    
    // Check if pawn is on or beyond 6th rank
    return (piece == P && rank >= 6) || (piece == p && rank >= 6);
}

// Null Move Pruning
bool SearchEnhanced::try_null_move(int depth, int beta) {
    // Don't use null move in endgame or very shallow searches
    if (depth < NMP_DEPTH_LIMIT) return false;
    
    // Don't use null move in check
    if (is_in_check()) return false;
    
    // Don't use null move with limited material
    int material = count_bits(bitboards[P]) + count_bits(bitboards[N]) + 
                  count_bits(bitboards[B]) + count_bits(bitboards[R]) + 
                  count_bits(bitboards[Q]) + count_bits(bitboards[p]) + 
                  count_bits(bitboards[n]) + count_bits(bitboards[b]) + 
                  count_bits(bitboards[r]) + count_bits(bitboards[q]);
    
    if (material < 6) return false;
    
    // Make null move
    copy_board();
    side ^= 1;
    hash_key ^= side_key;
    ply++;
    
    // Search with reduced depth
    int score = -negamax_enhanced(-beta + 1, -beta, depth - 3);
    
    // Undo null move
    undo_move();
    
    stats.null_move_cutoffs++;
    
    // If beta cutoff, try verification search
    if (score >= beta) {
        score = -negamax_enhanced(-beta, -beta + 1, depth - 3);
        if (score >= beta) {
            return true;
        }
    }
    
    return false;
}

// Futility Pruning
bool SearchEnhanced::is_futile_position(int alpha, int depth) {
    if (depth <= 1) {
        int eval = evaluate();
        return (eval + FUTILITY_MARGIN < alpha);
    }
    return false;
}

// Check Extension
int SearchEnhanced::get_check_extension() {
    return is_in_check() ? 1 : 0;
}

// Singular Extension
int SearchEnhanced::get_singular_extension(int move, int beta) {
    // Simplified singular extension check
    // In a full implementation, we would verify if this is the only move that beats alpha
    return 0; // Placeholder for now
}

// Multi-Cut Pruning
int SearchEnhanced::multi_cut(int alpha, int beta, int depth) {
    if (depth < 4) return beta;
    
    int cuts = 0;
    int beta_cutoff = beta;
    
    // Try multiple shallow searches
    for (int i = 0; i < 4; i++) {
        MoveList move_list;
        generate_moves(&move_list);
        
        if (move_list.count == 0) break;
        
        // Sort and get best few moves
        score_moves_enhanced(&move_list, depth);
        
        // Test first few moves
        for (int j = 0; j < 3 && j < move_list.count; j++) {
            int move = move_list.moves[j];
            
            if (!make_move(move, all_moves)) continue;
            
            repetition_table[repetition_index] = hash_key;
            repetition_index++;
            ply++;
            int score = -negamax_enhanced(-beta_cutoff, -alpha, depth - 2);
            undo_move();
            repetition_index--;
            
            if (score >= beta_cutoff) {
                cuts++;
                if (cuts >= 3) return beta_cutoff;
            }
            
            beta_cutoff = alpha + 1;
        }
    }
    
    return alpha;
}

// ProbCut (Probabilistic Pruning)
int SearchEnhanced::probcut(int beta, int depth) {
    if (depth < 5) return beta;
    
    // Use shallower search with reduced window
    int probcut_depth = depth / 2;
    int probcut_beta = beta - 50;
    
    MoveList move_list;
    generate_moves(&move_list);
    
    if (move_list.count == 0) return beta;
    
    score_moves_enhanced(&move_list, depth);
    
    // Test only best few moves
    for (int i = 0; i < 2 && i < move_list.count; i++) {
        int move = move_list.moves[i];
        
        if (!make_move(move, all_moves)) continue;
        
        repetition_table[repetition_index] = hash_key;
        repetition_index++;
        ply++;
        int score = -negamax_enhanced(-probcut_beta, -probcut_beta + 1, probcut_depth);
        undo_move();
        repetition_index--;
        
        if (score >= probcut_beta) {
            return beta;
        }
    }
    
    return beta;
}

// Same alpha-beta idea as search.cpp, with extra pruning (futility, multi-cut, probcut) and richer ordering.
int SearchEnhanced::negamax_enhanced(int alpha, int beta, int depth) {
    stats.nodes++;
    
    // Time management
    if (is_time_up()) return 0;
    
    // Draw detection
    if (is_draw() && ply) return 0;
    if (is_repetition()) return 0;
    
    // Quiescence search at leaf nodes
    if (depth == 0) return quiescence(alpha, beta);
    
    // Transposition table lookup
    int tt_score = 0, tt_move = 0;
    if (read_tt(hash_key, depth, alpha, beta, &tt_score, &tt_move)) {
        return tt_score;
    }
    
    // Futility pruning
    if (is_futile_position(alpha, depth)) {
        stats.futility_cutoffs++;
        return alpha;
    }
    
    // Multi-Cut pruning
    int beta_cutoff = multi_cut(alpha, beta, depth);
    if (beta_cutoff != beta) {
        stats.cutoffs++;
        return beta_cutoff;
    }
    
    // ProbCut
    beta_cutoff = probcut(beta, depth);
    if (beta_cutoff != beta) {
        stats.cutoffs++;
        return beta_cutoff;
    }
    
    // Generate and sort moves
    MoveList move_list;
    generate_moves(&move_list);
    
    if (move_list.count == 0) {
        // Checkmate or stalemate
        if (is_in_check()) {
            return -MATE_VALUE + ply;
        }
        return 0; // Stalemate
    }
    
    // Score moves with enhanced heuristics
    score_moves_enhanced(&move_list, depth);
    
    int legal_moves = 0;
    int best_score = -INFINITY;
    
    // Search moves
    for (int i = 0; i < move_list.count; i++) {
        int move = move_list.moves[i];
        
        if (!make_move(move, all_moves)) continue;
        
        legal_moves++;
        
        repetition_table[repetition_index] = hash_key;
        repetition_index++;
        ply++;
        
        // Check if we're in check before move
        bool in_check_before = is_in_check();
        
        // Get LMR reduction
        int lmr_reduction = get_lmr_reduction(depth, i, in_check_before);
        
        int new_depth = depth - 1 + get_check_extension();
        
        // Apply LMR
        if (lmr_reduction > 0 && !get_move_capture(move) && !get_move_promoted(move)) {
            new_depth -= lmr_reduction;
        }
        
        // Search
        int score = -negamax_enhanced(-beta, -alpha, new_depth);
        
        // Undo move
        undo_move();
        repetition_index--;
        
        // Time management
        if (is_time_up()) return 0;
        
        // Beta cutoff
        if (score >= beta) {
            write_tt(hash_key, depth, TT_BETA, beta, move);
            update_killer_moves(ply, move);
            update_history_scores(move, depth);
            stats.cutoffs++;
            return beta;
        }
        
        // Alpha improvement
        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                alpha = score;
            }
        }
    }
    
    // Store in transposition table
    if (best_score > alpha) {
        write_tt(hash_key, depth, TT_EXACT, best_score, move_list.moves[0]);
    } else {
        write_tt(hash_key, depth, TT_ALPHA, alpha, 0);
    }
    
    return best_score;
}

// Enhanced search wrapper
int search_position_enhanced() {
    // Reset statistics
    g_search_enhanced.reset_stats();
    
    // Clear move ordering and transposition table
    clear_move_ordering();
    clear_tt();
    
    // Iterative deepening with enhanced search
    MoveList pv;
    pv.count = 0;
    
    int best_move = 0;

    for (int current_depth = 1; current_depth <= search_info.depth_limit; current_depth++) {
        ply = 0;
        repetition_index = 0;
        
        int score = g_search_enhanced.negamax_enhanced(-INFINITY, INFINITY, current_depth);
        
        if (search_info.stopped) break;
        
        // Get PV
        get_pv_line(current_depth, &pv);
        if (pv.count > 0) {
            best_move = pv.moves[0];
        }
        
        // Print UCI-compliant thinking output.
        int time = get_time_ms() - search_info.starttime;
        printf("info depth %d score cp %d nodes %d time %d pv",
               current_depth, score, g_search_enhanced.get_stats().nodes, time);
        for (int i = 0; i < pv.count; i++) {
            printf(" ");
            print_move(pv.moves[i]);
        }
        printf("\n");
        fflush(stdout);
        
        // Check for mate
        if (abs(score) > MATE_SCORE) break;
    }
    
    return best_move;
}

} // namespace MissedClick
