#ifndef SEARCH_ENHANCED_HPP
#define SEARCH_ENHANCED_HPP

#include "search/search.hpp"
#include "board/types.hpp"

namespace MissedClick {

// Enhanced search with advanced pruning techniques
class SearchEnhanced {
private:
    static constexpr int LMR_DEPTH_LIMIT = 3;
    static constexpr int LMR_MOVES_LIMIT = 4;
    static constexpr int NMP_DEPTH_LIMIT = 2;
    static constexpr int FUTILITY_MARGIN = 100;

    // Internal helpers
    bool is_in_check();
    bool gives_check(int move);
    bool is_passed_pawn_move(int move);
    
public:
    // Enhanced negamax with all pruning techniques
    int negamax_enhanced(int alpha, int beta, int depth);
    
    // Late Move Reduction (LMR) - reduces search depth for later moves
    int get_lmr_reduction(int depth, int move_count, bool in_check);
    
    // Null Move Pruning (NMP) - skip opponent's move when position is winning
    bool try_null_move(int depth, int beta);
    
    // Futility Pruning - prune obviously bad moves
    bool is_futile_position(int alpha, int depth);
    
    // Check Extensions - extend search for checks
    int get_check_extension();
    
    // Singular Extensions - extend search for singular moves
    int get_singular_extension(int move, int beta);
    
    // Multi-Cut Pruning - try multiple cutoffs at once
    int multi_cut(int alpha, int beta, int depth);
    
    // ProbCut - probabilistic pruning for deep searches
    int probcut(int beta, int depth);
    
    // Enhanced move ordering with more heuristics
    void score_moves_enhanced(MoveList* move_list, int depth);
    
    // Time management optimizations
    void allocate_time(int time_left, int movestogo);
    
    // Search statistics
    struct SearchStats {
        int nodes;
        int cutoffs;
        int null_move_cutoffs;
        int futility_cutoffs;
        int lmr_reductions;
        int extensions;
    };
    
    SearchStats stats;
    
    // Reset statistics
    void reset_stats();
    
    // Get statistics
    const SearchStats& get_stats() const { return stats; }
};

// Global enhanced search instance
extern SearchEnhanced g_search_enhanced;

// Enhanced search wrapper
int search_position_enhanced();

} // namespace MissedClick

#endif // SEARCH_ENHANCED_HPP
