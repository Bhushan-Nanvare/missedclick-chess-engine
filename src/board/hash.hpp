#ifndef HASH_HPP
#define HASH_HPP

#include "board/types.hpp"
#include "utils/utils.hpp"
#include "board/bitboard.hpp"

// Forward declarations for board state
namespace MissedClick {
    extern U64 bitboards[12];
    extern U64 occupancies[3];
    extern int side;
    extern int enpassant;
    extern int castle;
}

// Zobrist hash keys
extern U64 piece_keys[12][64];
extern U64 enpassant_keys[64];
extern U64 castle_keys[16];
extern U64 side_key;

// Initialize random hash keys
void init_random_keys();

// Generate hash key from current position
U64 generate_hash_key();

#endif // HASH_HPP
