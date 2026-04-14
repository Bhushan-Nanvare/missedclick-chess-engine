#ifndef EVALUATION_ENHANCED_HPP
#define EVALUATION_ENHANCED_HPP

#include "eval/evaluate.hpp"
#include "board/types.hpp"

namespace MissedClick {

// Enhanced evaluation with advanced features
class EvaluationEnhanced {
private:
    static constexpr int EVAL_CACHE_SIZE = 1024;
    static constexpr int PAWN_HASH_SIZE = 16384;
    
    // Evaluation cache
    struct EvalCacheEntry {
        U64 hash_key;
        int score;
        uint8_t depth;
        bool valid;
    };
    
    // Pawn hash table for pawn structure evaluation
    struct PawnHashEntry {
        U64 hash_key;
        int pawn_score;
        int passed_pawns[2];  // Number of passed pawns for each side
        bool valid;
    };
    
    EvalCacheEntry eval_cache[EVAL_CACHE_SIZE];
    PawnHashEntry pawn_hash[PAWN_HASH_SIZE];
    
    // Statistics
    int cache_hits;
    int cache_misses;
    int pawn_hash_hits;
    int pawn_hash_misses;
    
public:
    EvaluationEnhanced() : cache_hits(0), cache_misses(0), pawn_hash_hits(0), pawn_hash_misses(0) {
        for (int i = 0; i < EVAL_CACHE_SIZE; i++) {
            eval_cache[i].valid = false;
        }
        for (int i = 0; i < PAWN_HASH_SIZE; i++) {
            pawn_hash[i].valid = false;
        }
    }
    
    // Enhanced evaluation with caching
    int evaluate_position();
    
    // Material evaluation with piece-square tables
    int evaluate_material();
    
    // Positional evaluation with advanced heuristics
    int evaluate_positional();
    
    // Pawn structure evaluation with hashing
    int evaluate_pawn_structure();
    
    // King safety evaluation
    int evaluate_king_safety();
    
    // Piece mobility evaluation
    int evaluate_mobility();
    
    // Piece coordination evaluation
    int evaluate_coordination();
    
    // Endgame evaluation
    int evaluate_endgame();
    
    // Clear caches
    void clear_caches();
    
    // Get cache statistics
    double get_cache_hit_rate() const;
    double get_pawn_hash_hit_rate() const;
    
    // Print statistics
    void print_stats();
};

// Global enhanced evaluator
extern EvaluationEnhanced g_evaluator_enhanced;

// Enhanced evaluation wrapper
int evaluate_enhanced();

} // namespace MissedClick

#endif // EVALUATION_ENHANCED_HPP
