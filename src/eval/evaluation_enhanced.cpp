#include "eval/evaluation_enhanced.hpp"
#include "board/board.hpp"
#include "board/bitboard.hpp"
#include "attacks/attacks.hpp"
#include "utils/utils.hpp"
#include "board/bitboard_optimized.hpp"
#include <cstdio>

namespace MissedClick {

// Global enhanced evaluator
EvaluationEnhanced g_evaluator_enhanced;

int EvaluationEnhanced::evaluate_position() {
    int index = hash_key % EVAL_CACHE_SIZE;

    // Cheap eval cache: same pawn structure / material often repeats in search.
    if (eval_cache[index].valid && eval_cache[index].hash_key == hash_key) {
        cache_hits++;
        return eval_cache[index].score;
    }
    
    cache_misses++;
    
    // Compute evaluation
    int score = 0;
    
    // Material evaluation
    score += evaluate_material();
    
    // Positional evaluation
    score += evaluate_positional();
    
    // Pawn structure
    score += evaluate_pawn_structure();
    
    // King safety
    score += evaluate_king_safety();
    
    // Mobility
    score += evaluate_mobility();
    
    // Coordination
    score += evaluate_coordination();
    
    // Endgame adjustments
    score += evaluate_endgame();
    
    // Store in cache
    eval_cache[index] = {hash_key, score, 0, true};
    
    return score;
}

// Material evaluation with piece-square tables
int EvaluationEnhanced::evaluate_material() {
    int score = 0;
    
    // Basic material values
    const int material_values[12] = {
        100, 325, 335, 500, 975, 20000,  // P,N,B,R,Q,K (white)
        -100, -325, -335, -500, -975, -20000  // p,n,b,r,q,k (black)
    };
    
    // Piece-square tables for positional bonuses
    const int pawn_pst[64] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5,  5, 10, 25, 25, 10,  5,  5,
        0,  0,  0, 20, 20,  0,  0,  0,
        5, -5,-10,  0,  0,-10, -5,  5,
        5, 10, 10,-20,-20, 10, 10,  5,
        0,  0,  0,  0,  0,  0,  0,  0
    };
    
    const int knight_pst[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };
    
    // Calculate material and positional score
    for (int piece = P; piece <= K; piece++) {
        U64 pieces = bitboards[piece];
        while (pieces) {
            int square = get_ls1b_index_optimized(pieces);
            score += material_values[piece];
            
            // Add positional bonus
            if (piece == P) {
                score += pawn_pst[square];
            } else if (piece == N) {
                score += knight_pst[square];
            }
            
            POP_BIT(pieces, square);
        }
    }
    
    for (int piece = p; piece <= k; piece++) {
        U64 pieces = bitboards[piece];
        while (pieces) {
            int square = get_ls1b_index_optimized(pieces);
            score += material_values[piece];
            
            // Add positional bonus (mirrored for black)
            if (piece == p) {
                score -= pawn_pst[63 - square];
            } else if (piece == n) {
                score -= knight_pst[63 - square];
            }
            
            POP_BIT(pieces, square);
        }
    }
    
    return score;
}

// Positional evaluation
int EvaluationEnhanced::evaluate_positional() {
    int score = 0;
    
    // Center control
    U64 center_squares = (1ULL << d4) | (1ULL << e4) | (1ULL << d5) | (1ULL << e5);
    U64 white_center = occupancies[white] & center_squares;
    U64 black_center = occupancies[black] & center_squares;
    
    score += 20 * (count_bits_optimized(white_center) - count_bits_optimized(black_center));
    
    // Bishop pair bonus
    if (count_bits_optimized(bitboards[B]) >= 2) score += 50;
    if (count_bits_optimized(bitboards[b]) >= 2) score -= 50;
    
    // Knight outpost bonus
    U64 white_knights = bitboards[N];
    while (white_knights) {
        int square = get_ls1b_index_optimized(white_knights);
        int rank = square / 8;
        int file = square % 8;
        
        // Knight on 5th or 6th rank, supported by pawn
        if (rank >= 4 && rank <= 5 && file >= 2 && file <= 5) {
            U64 supporting_pawns = bitboards[P] & 
                ((1ULL << (square - 7)) | (1ULL << (square - 9)) | (1ULL << (square + 7)) | (1ULL << (square + 9)));
            if (supporting_pawns) score += 30;
        }
        
        POP_BIT(white_knights, square);
    }
    
    return score;
}

// Pawn structure evaluation with hashing
int EvaluationEnhanced::evaluate_pawn_structure() {
    // Calculate pawn hash
    U64 pawn_key = hash_key ^ (bitboards[P] | bitboards[p]);
    int index = pawn_key % PAWN_HASH_SIZE;
    
    // Check pawn hash
    if (pawn_hash[index].valid && pawn_hash[index].hash_key == pawn_key) {
        pawn_hash_hits++;
        return pawn_hash[index].pawn_score;
    }
    
    pawn_hash_misses++;
    
    int score = 0;
    int white_passed = 0;
    int black_passed = 0;
    
    // Evaluate white pawns
    U64 white_pawns = bitboards[P];
    while (white_pawns) {
        int square = get_ls1b_index_optimized(white_pawns);
        int rank = square / 8;
        int file = square % 8;
        
        // Check if pawn is passed
        bool is_passed = true;
        U64 enemy_pawns = bitboards[p];
        
        // Check files ahead
        for (int f = file - 1; f <= file + 1; f++) {
            if (f < 0 || f > 7) continue;
            
            U64 file_mask = 0xFFULL << (f * 8);
            U64 enemy_pawns_in_file = enemy_pawns & file_mask;
            
            // Check if any enemy pawn is ahead
            for (int r = rank + 1; r < 8; r++) {
                if (enemy_pawns_in_file & (1ULL << (r * 8 + f))) {
                    is_passed = false;
                    break;
                }
            }
            if (!is_passed) break;
        }
        
        if (is_passed) {
            white_passed++;
            score += 50 + rank * 10;  // Bonus for passed pawns
        }
        
        // Doubled pawn penalty
        U64 file_mask = 0xFFULL << (file * 8);
        U64 pawns_in_file = bitboards[P] & file_mask;
        if (count_bits_optimized(pawns_in_file) > 1) {
            score -= 20;
        }
        
        // Isolated pawn penalty
        bool isolated = true;
        if (file > 0 && (bitboards[P] & (0xFFULL << ((file - 1) * 8)))) isolated = false;
        if (file < 7 && (bitboards[P] & (0xFFULL << ((file + 1) * 8)))) isolated = false;
        if (isolated) score -= 15;
        
        POP_BIT(white_pawns, square);
    }
    
    // Evaluate black pawns (similar logic)
    U64 black_pawns = bitboards[p];
    while (black_pawns) {
        int square = get_ls1b_index_optimized(black_pawns);
        int rank = 7 - (square / 8);  // Invert rank for black
        int file = square % 8;
        
        // Check if pawn is passed
        bool is_passed = true;
        U64 enemy_pawns = bitboards[P];
        
        for (int f = file - 1; f <= file + 1; f++) {
            if (f < 0 || f > 7) continue;
            
            U64 file_mask = 0xFFULL << (f * 8);
            U64 enemy_pawns_in_file = enemy_pawns & file_mask;
            
            for (int r = rank + 1; r < 8; r++) {
                int black_rank = 7 - r;
                if (enemy_pawns_in_file & (1ULL << (black_rank * 8 + f))) {
                    is_passed = false;
                    break;
                }
            }
            if (!is_passed) break;
        }
        
        if (is_passed) {
            black_passed++;
            score -= (50 + rank * 10);
        }
        
        // Doubled pawn penalty
        U64 file_mask = 0xFFULL << (file * 8);
        U64 pawns_in_file = bitboards[p] & file_mask;
        if (count_bits_optimized(pawns_in_file) > 1) {
            score += 20;
        }
        
        // Isolated pawn penalty
        bool isolated = true;
        if (file > 0 && (bitboards[p] & (0xFFULL << ((file - 1) * 8)))) isolated = false;
        if (file < 7 && (bitboards[p] & (0xFFULL << ((file + 1) * 8)))) isolated = false;
        if (isolated) score += 15;
        
        POP_BIT(black_pawns, square);
    }
    
    // Store in pawn hash
    pawn_hash[index] = {pawn_key, score, {white_passed, black_passed}, true};
    
    return score;
}

// King safety evaluation
int EvaluationEnhanced::evaluate_king_safety() {
    int score = 0;
    
    // White king safety
    U64 white_king = bitboards[K];
    int wk_square = get_ls1b_index_optimized(white_king);
    
    // Check if king is in starting position (unsafe)
    if (wk_square == e1) {
        // Penalty for not castling
        if (castle & (wk | wq)) {
            score -= 30;
        }
    }
    
    // King shelter evaluation
    int wk_rank = wk_square / 8;
    int wk_file = wk_square / 8;
    
    // Evaluate pawn shield
    if (wk_rank <= 2) {  // King on back rank or one rank up
        for (int f = wk_file - 1; f <= wk_file + 1; f++) {
            if (f < 0 || f > 7) continue;
            
            U64 pawn_square = 1ULL << ((wk_rank + 1) * 8 + f);
            if (bitboards[P] & pawn_square) {
                score += 10;  // Pawn in front of king
            } else {
                score -= 15;  // Open file in front of king
            }
        }
    }
    
    // Black king safety (similar logic)
    U64 black_king = bitboards[k];
    int bk_square = get_ls1b_index_optimized(black_king);
    
    if (bk_square == e8) {
        if (castle & (bk | bq)) {
            score += 30;
        }
    }
    
    int bk_rank = 7 - (bk_square / 8);
    int bk_file = bk_square / 8;
    
    if (bk_rank <= 2) {
        for (int f = bk_file - 1; f <= bk_file + 1; f++) {
            if (f < 0 || f > 7) continue;
            
            int black_pawn_rank = 7 - (bk_rank + 1);
            U64 pawn_square = 1ULL << (black_pawn_rank * 8 + f);
            if (bitboards[p] & pawn_square) {
                score -= 10;
            } else {
                score += 15;
            }
        }
    }
    
    return score;
}

// Mobility evaluation
int EvaluationEnhanced::evaluate_mobility() {
    int score = 0;
    
    // Knight mobility
    U64 white_knights = bitboards[N];
    while (white_knights) {
        int square = get_ls1b_index_optimized(white_knights);
        U64 attacks = knight_attacks[square];
        int mobility = count_bits_optimized(attacks & ~occupancies[white]);
        score += mobility * 4;  // 4 points per legal move
        POP_BIT(white_knights, square);
    }
    
    U64 black_knights = bitboards[n];
    while (black_knights) {
        int square = get_ls1b_index_optimized(black_knights);
        U64 attacks = knight_attacks[square];
        int mobility = count_bits_optimized(attacks & ~occupancies[black]);
        score -= mobility * 4;
        POP_BIT(black_knights, square);
    }
    
    // Bishop mobility
    U64 white_bishops = bitboards[B];
    while (white_bishops) {
        int square = get_ls1b_index_optimized(white_bishops);
        U64 attacks = get_bishop_attacks(square, occupancies[both]);
        int mobility = count_bits_optimized(attacks & ~occupancies[white]);
        score += mobility * 3;  // 3 points per legal move
        POP_BIT(white_bishops, square);
    }
    
    U64 black_bishops = bitboards[b];
    while (black_bishops) {
        int square = get_ls1b_index_optimized(black_bishops);
        U64 attacks = get_bishop_attacks(square, occupancies[both]);
        int mobility = count_bits_optimized(attacks & ~occupancies[black]);
        score -= mobility * 3;
        POP_BIT(black_bishops, square);
    }
    
    return score;
}

// Piece coordination evaluation
int EvaluationEnhanced::evaluate_coordination() {
    int score = 0;
    
    // Queen-knight coordination
    U64 white_queen = bitboards[Q];
    U64 white_knights = bitboards[N];
    
    if (white_queen && white_knights) {
        int q_square = get_ls1b_index_optimized(white_queen);
        U64 q_attacks = get_queen_attacks(q_square, occupancies[both]);
        
        U64 knights = white_knights;
        while (knights) {
            int n_square = get_ls1b_index_optimized(knights);
            U64 n_attacks = knight_attacks[n_square];
            
            // Bonus for coordinated attacks
            if (q_attacks & n_attacks) {
                score += 20;
            }
            
            POP_BIT(knights, n_square);
        }
    }
    
    // Rook-pawn coordination
    U64 white_rooks = bitboards[R];
    U64 white_pawns = bitboards[P];
    
    while (white_rooks) {
        int r_square = get_ls1b_index_optimized(white_rooks);
        int r_file = r_square % 8;
        
        // Check if rook has supporting pawns
        U64 file_pawns = white_pawns & (0xFFULL << (r_file * 8));
        if (file_pawns) {
            score += 15;  // Rook supported by pawns
        }
        
        POP_BIT(white_rooks, r_square);
    }
    
    return score;
}

// Endgame evaluation
int EvaluationEnhanced::evaluate_endgame() {
    int score = 0;
    
    // Check if we're in endgame
    int material = count_bits_optimized(bitboards[P]) + count_bits_optimized(bitboards[N]) + 
                  count_bits_optimized(bitboards[B]) + count_bits_optimized(bitboards[R]) + 
                  count_bits_optimized(bitboards[Q]);
    
    if (material < 10) {  // Endgame threshold
        // King activity becomes more important
        U64 white_king = bitboards[K];
        U64 black_king = bitboards[k];
        
        int wk_square = get_ls1b_index_optimized(white_king);
        int bk_square = get_ls1b_index_optimized(black_king);
        
        // Center king bonus in endgame
        U64 center = (1ULL << d4) | (1ULL << e4) | (1ULL << d5) | (1ULL << e5);
        if (white_king & center) score += 20;
        if (black_king & center) score -= 20;
        
        // Opposition bonus
        int wk_rank = wk_square / 8;
        int bk_rank = bk_square / 8;
        if (wk_rank > bk_rank) score += 10;
        else score -= 10;
    }
    
    return score;
}

// Clear caches
void EvaluationEnhanced::clear_caches() {
    for (int i = 0; i < EVAL_CACHE_SIZE; i++) {
        eval_cache[i].valid = false;
    }
    for (int i = 0; i < PAWN_HASH_SIZE; i++) {
        pawn_hash[i].valid = false;
    }
    
    cache_hits = cache_misses = pawn_hash_hits = pawn_hash_misses = 0;
}

// Get cache statistics
double EvaluationEnhanced::get_cache_hit_rate() const {
    int total = cache_hits + cache_misses;
    return total > 0 ? (double)cache_hits / total : 0.0;
}

double EvaluationEnhanced::get_pawn_hash_hit_rate() const {
    int total = pawn_hash_hits + pawn_hash_misses;
    return total > 0 ? (double)pawn_hash_hits / total : 0.0;
}

// Print statistics
void EvaluationEnhanced::print_stats() {
    printf("Evaluation Statistics:\n");
    printf("  Cache hit rate: %.1f%%\n", get_cache_hit_rate() * 100);
    printf("  Pawn hash hit rate: %.1f%%\n", get_pawn_hash_hit_rate() * 100);
    printf("  Cache hits: %d, misses: %d\n", cache_hits, cache_misses);
    printf("  Pawn hash hits: %d, misses: %d\n", pawn_hash_hits, pawn_hash_misses);
}

// Enhanced evaluation wrapper
int evaluate_enhanced() {
    return g_evaluator_enhanced.evaluate_position();
}

} // namespace MissedClick
