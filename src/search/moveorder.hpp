#ifndef MOVEORDER_HPP
#define MOVEORDER_HPP

#include "board/types.hpp"
#include "moves/movegen.hpp"

namespace MissedClick {

// Move ordering tables (defined in moveorder.cpp)
extern const int mvv_lva_scores[12][12];
extern int killer_moves[2][600];
extern int history_scores[12][64];

// Clear killer moves and history scores
void clear_move_ordering();

// Update killer moves
void update_killer_moves(int ply, int move);

// Update history scores
void update_history_scores(int move, int depth);

// Score move for ordering
int score_move(int move);

// Sort moves by score (insertion sort for efficiency)
void sort_moves(MoveList* move_list);

// Get principal variation move from transposition table
int get_pv_move();

} // namespace MissedClick

#endif // MOVEORDER_HPP
