#include "search/tt.hpp"
#include "board/board.hpp"
#include <cstring>

namespace MissedClick {

// Global transposition table
TTEntry tt[TT_SIZE];

// Clear transposition table
void clear_tt() {
    std::memset(tt, 0, sizeof(tt));
}

// Read entry from transposition table
bool read_tt(U64 key, int depth, int alpha, int beta, int* score, int* move) {
    // Get index from hash key
    int index = key % TT_SIZE;
    
    // Make sure we have the right entry
    if (tt[index].key != key)
        return false;
    
    // Make sure entry depth is sufficient
    if (tt[index].depth < depth)
        return false;
    
    // Get score and move
    *score = tt[index].score;
    *move = tt[index].move;
    
    // Return true if score causes a cutoff
    if (tt[index].flags == TT_ALPHA && *score <= alpha)
        return true;
    
    if (tt[index].flags == TT_BETA && *score >= beta)
        return true;
    
    if (tt[index].flags == TT_EXACT)
        return true;
    
    return false;
}

// Write entry to transposition table
void write_tt(U64 key, int depth, int flags, int score, int move) {
    // Get index from hash key
    int index = key % TT_SIZE;
    
    // Store entry
    tt[index].key = key;
    tt[index].depth = depth;
    tt[index].flags = flags;
    tt[index].score = score;
    tt[index].move = move;
}

// Get transposition table entry count
int get_tt_count() {
    int count = 0;
    for (int i = 0; i < TT_SIZE; i++) {
        if (tt[i].key != 0)
            count++;
    }
    return count;
}

// Resize transposition table (for different memory sizes)
void resize_tt(int size_mb) {
    // This would require dynamic allocation
    // For now, we'll use the fixed size
    // In a full implementation, you would:
    // 1. Calculate number of entries from size_mb
    // 2. Free current table
    // 3. Allocate new table
    // 4. Clear new table
}

} // namespace MissedClick
