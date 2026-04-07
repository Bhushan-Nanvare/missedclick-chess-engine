#include "moves/makemove.hpp"
#include "board/bitboard.hpp"
#include "attacks/attacks.hpp"
#include "moves/movegen.hpp"
#include "board/hash.hpp"
#include "board/board.hpp"
#include <cstring>

using namespace MissedClick;

namespace {
    constexpr int MAX_STATE_PLY = 1024;
    MissedClick::BoardState board_state_stack[MAX_STATE_PLY];
}

// Save board state at current ply index (search calls this before making a move)
void MissedClick::copy_board() {
    int idx = MissedClick::ply;
    if (idx < 0) idx = 0;
    if (idx >= MAX_STATE_PLY) idx = MAX_STATE_PLY - 1;

    memcpy(board_state_stack[idx].bitboards_copy, MissedClick::bitboards, sizeof(MissedClick::bitboards));
    memcpy(board_state_stack[idx].occupancies_copy, MissedClick::occupancies, sizeof(MissedClick::occupancies));
    board_state_stack[idx].side_copy       = MissedClick::side;
    board_state_stack[idx].enpassant_copy  = MissedClick::enpassant;
    board_state_stack[idx].castle_copy     = MissedClick::castle;
    board_state_stack[idx].fifty_copy      = MissedClick::fifty;
    board_state_stack[idx].hash_key_copy   = MissedClick::hash_key;
}

static inline void restore_from(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= MAX_STATE_PLY) idx = MAX_STATE_PLY - 1;
    memcpy(MissedClick::bitboards, board_state_stack[idx].bitboards_copy, sizeof(MissedClick::bitboards));
    memcpy(MissedClick::occupancies, board_state_stack[idx].occupancies_copy, sizeof(MissedClick::occupancies));
    MissedClick::side       = board_state_stack[idx].side_copy;
    MissedClick::enpassant  = board_state_stack[idx].enpassant_copy;
    MissedClick::castle     = board_state_stack[idx].castle_copy;
    MissedClick::fifty      = board_state_stack[idx].fifty_copy;
    MissedClick::hash_key   = board_state_stack[idx].hash_key_copy;
}

// Restore after a successful move.
// Caller already incremented ply after make_move, so we decrement it here
// and restore the saved board state from that index.
void MissedClick::undo_move() {
    MissedClick::ply--;
    if (MissedClick::ply < 0) MissedClick::ply = 0;
    restore_from(MissedClick::ply);
}

// Restore after a rejected illegal move (ply is unchanged; restore from current ply)
void MissedClick::restore_illegal_move() {
    int idx = MissedClick::ply;
    restore_from(idx);
}

int MissedClick::make_move(int move, int move_flag) {
    if (move_flag == all_moves) {
        // Save board state at current ply before any changes
        copy_board();

        int source_square   = get_move_source(move);
        int target_square   = get_move_target(move);
        int piece           = get_move_piece(move);
        int promoted_piece  = get_move_promoted(move);
        int capture         = get_move_capture(move);
        int double_push     = get_move_double(move);
        int enpass          = get_move_enpassant(move);
        int castling        = get_move_castling(move);

        // Move piece: remove from source, place on target
        POP_BIT(MissedClick::bitboards[piece], source_square);
        SET_BIT(MissedClick::bitboards[piece], target_square);

        // Update hash for piece movement
        MissedClick::hash_key ^= piece_keys[piece][source_square];
        MissedClick::hash_key ^= piece_keys[piece][target_square];

        // Update fifty-move counter
        MissedClick::fifty++;
        if (piece == P || piece == p)
            MissedClick::fifty = 0;

        // Handle captures
        if (capture) {
            MissedClick::fifty = 0;
            int start_piece = (MissedClick::side == white) ? p : P;
            int end_piece   = (MissedClick::side == white) ? k : K;
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++) {
                if (GET_BIT(MissedClick::bitboards[bb_piece], target_square)) {
                    POP_BIT(MissedClick::bitboards[bb_piece], target_square);
                    MissedClick::hash_key ^= piece_keys[bb_piece][target_square];
                    break;
                }
            }
        }

        // Handle promotions
        if (promoted_piece) {
            if (side == white) {
                POP_BIT(bitboards[P], target_square);
                hash_key ^= piece_keys[P][target_square];
            } else {
                POP_BIT(bitboards[p], target_square);
                hash_key ^= piece_keys[p][target_square];
            }
            SET_BIT(bitboards[promoted_piece], target_square);
            hash_key ^= piece_keys[promoted_piece][target_square];
        }

        // Handle en passant capture
        if (enpass) {
            if (side == white) {
                POP_BIT(bitboards[p], target_square + 8);
                hash_key ^= piece_keys[p][target_square + 8];
            } else {
                POP_BIT(bitboards[P], target_square - 8);
                hash_key ^= piece_keys[P][target_square - 8];
            }
        }

        // Remove old en passant from hash
        if (enpassant != no_sq) hash_key ^= enpassant_keys[enpassant];
        enpassant = no_sq;

        // Handle double pawn push (set new en passant square)
        if (double_push) {
            if (side == white) {
                enpassant = target_square + 8;
            } else {
                enpassant = target_square - 8;
            }
            hash_key ^= enpassant_keys[enpassant];
        }

        // Handle castling: move the rook
        if (castling) {
            switch (target_square) {
                case g1:  // white kingside
                    POP_BIT(bitboards[R], h1);
                    SET_BIT(bitboards[R], f1);
                    hash_key ^= piece_keys[R][h1];
                    hash_key ^= piece_keys[R][f1];
                    break;
                case c1:  // white queenside
                    POP_BIT(bitboards[R], a1);
                    SET_BIT(bitboards[R], d1);
                    hash_key ^= piece_keys[R][a1];
                    hash_key ^= piece_keys[R][d1];
                    break;
                case g8:  // black kingside
                    POP_BIT(bitboards[r], h8);
                    SET_BIT(bitboards[r], f8);
                    hash_key ^= piece_keys[r][h8];
                    hash_key ^= piece_keys[r][f8];
                    break;
                case c8:  // black queenside
                    POP_BIT(bitboards[r], a8);
                    SET_BIT(bitboards[r], d8);
                    hash_key ^= piece_keys[r][a8];
                    hash_key ^= piece_keys[r][d8];
                    break;
            }
        }

        // Update castling rights in hash
        hash_key ^= castle_keys[castle];
        castle &= castling_rights[source_square];
        castle &= castling_rights[target_square];
        hash_key ^= castle_keys[castle];

        // Recompute occupancies
        memset(occupancies, 0, sizeof(occupancies));
        for (int bb_piece = P; bb_piece <= K; bb_piece++)
            occupancies[white] |= bitboards[bb_piece];
        for (int bb_piece = p; bb_piece <= k; bb_piece++)
            occupancies[black] |= bitboards[bb_piece];
        occupancies[both] = occupancies[white] | occupancies[black];

        // Switch side
        side ^= 1;
        hash_key ^= side_key;

        // Verify the king is not left in check (legality check)
        // After side flip, check if the side that just moved left their king in check
        int king_sq = (side == white)
            ? get_ls1b_index(bitboards[k])   // black king (just moved)
            : get_ls1b_index(bitboards[K]);  // white king (just moved)

        if (is_square_attacked(king_sq, side)) {
            // Illegal move: restore
            restore_illegal_move();
            return 0;
        }

        return 1;
    }
    else {
        // only_captures mode
        if (get_move_capture(move))
            return make_move(move, all_moves);
        return 0;
    }
}
