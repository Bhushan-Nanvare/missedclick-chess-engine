#ifndef BOARD_HPP
#define BOARD_HPP

#include "board/types.hpp"
#include "board/bitboard.hpp"
#include <cstring>

// Forward declaration for hash key generation
U64 generate_hash_key();

// Per-square mask used when a piece moves: AND castle with castling_rights[sq] to drop
// rights for any rook/king that left its home square (see makemove).
extern const int castling_rights[64];

// Global board: twelve bitboards (P..K white, p..k black), one uint64_t per piece type.
// occupancies[white/black/both] are OR-combined masks for ray attacks and legality.
namespace MissedClick {
    extern U64 bitboards[12];
    
    extern U64 occupancies[3];
    
    // Side to move
    extern int side;
    
    // Enpassant square
    extern int enpassant;
    
    // Castling rights
    extern int castle;
    
    // Hash key
    extern U64 hash_key;
    
    // Repetition table
    extern U64 repetition_table[1000];
    
    // Repetition index
    extern int repetition_index;
    
    // Half move counter (ply)
    extern int ply;
    
    // Fifty move rule counter
    extern int fifty;
    
    // Snapshot for make/undo (search stack uses a deeper copy in makemove.cpp).
    struct BoardState {
        U64 bitboards_copy[12];
        U64 occupancies_copy[3];
        int side_copy;
        int enpassant_copy;
        int castle_copy;
        int fifty_copy;
        U64 hash_key_copy;
    };

    // Board state backup storage (defined in board.cpp)
    extern BoardState board_state;
    
    // Reset board to empty position
    void reset_board();
    
    // Parse FEN string
    void parse_fen(const char* fen);
    
    // Print board (for debugging)
    void print_board();
    
    // Print bitboard (for debugging)
    void print_bitboard(U64 bitboard);
    
    // Copy board state (for move making)
    inline void copy_board(BoardState& state) {
        std::memcpy(state.bitboards_copy, bitboards, sizeof(bitboards));
        std::memcpy(state.occupancies_copy, occupancies, sizeof(occupancies));
        state.side_copy = side;
        state.enpassant_copy = enpassant;
        state.castle_copy = castle;
        state.fifty_copy = fifty;
        state.hash_key_copy = hash_key;
    }
    
    // Restore board state (for move unmaking)
    inline void take_back(const BoardState& state) {
        std::memcpy(bitboards, state.bitboards_copy, sizeof(bitboards));
        std::memcpy(occupancies, state.occupancies_copy, sizeof(occupancies));
        side = state.side_copy;
        enpassant = state.enpassant_copy;
        castle = state.castle_copy;
        fifty = state.fifty_copy;
        hash_key = state.hash_key_copy;
    }
}

#endif // BOARD_HPP
