#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "board/types.hpp"

namespace MissedClick {

// Hierarchical score from the side to move (positive = good for that side).
int evaluate();

// Convert piece positions for NNUE evaluation
void nnue_refresh();

// Check if position is a draw
bool is_draw();

// Check if position is a checkmate
bool is_checkmate();

// Check if position is a stalemate
bool is_stalemate();

// Get material balance
int get_material_balance();

// Get piece-square table score
int get_piece_square_score();

// Endgame evaluation
int evaluate_endgame();

// Opening evaluation
int evaluate_opening();

// Middlegame evaluation
int evaluate_middlegame();

} // namespace MissedClick

#endif // EVALUATE_HPP
