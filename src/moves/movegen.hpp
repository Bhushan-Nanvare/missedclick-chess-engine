#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include "board/types.hpp"
#include "board/bitboard.hpp"
#include "attacks/attacks.hpp"
#include "board/board.hpp"
#include <cstdio>

namespace MissedClick {

// Pseudo-legal generation: pinned pieces and king-in-check are filtered in make_move().

// Add move to move list
inline void add_move(MoveList* move_list, int move) {
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

// Check if square is attacked by given side
bool is_square_attacked(int square, int side);

// Generate all moves
void generate_moves(MoveList* move_list);

// Print move list (for debugging)
void print_move_list(MoveList* move_list);

} // namespace MissedClick

#endif // MOVEGEN_HPP
