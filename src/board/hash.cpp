#include "board/hash.hpp"
#include "board/board.hpp"
#include "utils/utils.hpp"
#include "board/bitboard.hpp"

// Zobrist hash keys
U64 piece_keys[12][64];
U64 enpassant_keys[64];
U64 castle_keys[16];
U64 side_key;

// Initialize random hash keys
void init_random_keys() {
    // Update pseudo random number state
    // Note: random_state is static in utils.cpp, so we reset it here
    // by calling get_random_U64_number after initializing
    
    // Loop over piece codes
    for (int piece = P; piece <= k; piece++) {
        // Loop over board squares
        for (int square = 0; square < 64; square++) {
            // Init random piece keys
            piece_keys[piece][square] = get_random_U64_number();
        }
    }
    
    // Loop over board squares
    for (int square = 0; square < 64; square++) {
        // Init random enpassant keys
        enpassant_keys[square] = get_random_U64_number();
    }
    
    // Loop over castling keys
    for (int index = 0; index < 16; index++) {
        // Init castling keys
        castle_keys[index] = get_random_U64_number();
    }
    
    // Init random side key
    side_key = get_random_U64_number();
}

// Generate hash key from current position
U64 generate_hash_key() {
    using namespace MissedClick;
    
    U64 final_key = 0ULL;
    U64 bitboard;
    
    // Loop over piece bitboards
    for (int piece = P; piece <= k; piece++) {
        bitboard = bitboards[piece];
        
        while (bitboard) {
            int square = get_ls1b_index(bitboard);
            final_key ^= piece_keys[piece][square];
            POP_BIT(bitboard, square);
        }
    }
    
    // If enpassant square is on board
    if (enpassant != no_sq) {
        final_key ^= enpassant_keys[enpassant];
    }
    
    // Hash castling rights
    final_key ^= castle_keys[castle];
    
    // Hash the side only if black is to move
    if (side == black) {
        final_key ^= side_key;
    }
    
    return final_key;
}
