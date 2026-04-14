#ifndef TT_HPP
#define TT_HPP

#include "board/types.hpp"

namespace MissedClick {

// Transposition table entry
struct TTEntry {
    U64 key;        // Zobrist hash key
    int depth;      // Search depth
    int flags;      // Entry type (exact, alpha, beta)
    int score;      // Evaluation score
    int move;       // Best move
};

// Transposition table hash flags
enum TTFlag {
    TT_ALPHA,   // Upper bound (fail-low)
    TT_BETA,    // Lower bound (fail-high) 
    TT_EXACT    // Exact score
};

// Transposition table size (in entries)
#define TT_SIZE 1048583  // Prime number for better hashing

// Clear transposition table
void clear_tt();

// Read entry from transposition table
bool read_tt(U64 key, int depth, int alpha, int beta, int* score, int* move);

// Write entry to transposition table
void write_tt(U64 key, int depth, int flags, int score, int move);

// Get transposition table entry count
int get_tt_count();

// Resize transposition table (for different memory sizes)
void resize_tt(int size_mb);

} // namespace MissedClick

#endif // TT_HPP
