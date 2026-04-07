#ifndef MAKEMOVE_HPP
#define MAKEMOVE_HPP

#include "board/types.hpp"
#include "board/board.hpp"

namespace MissedClick {

// Copy board state before attempting a move (indexed by current ply).
void copy_board();

// Undo a successful make_move after search increments ply (restores stack[ply-1] after ply--).
void undo_move();

// Restore board after a rejected illegal move (ply unchanged; restores stack[ply]).
void restore_illegal_move();

// Make move on chess board
// Returns 1 for legal move, 0 for illegal move
int make_move(int move, int move_flag);

} // namespace MissedClick

#endif // MAKEMOVE_HPP
