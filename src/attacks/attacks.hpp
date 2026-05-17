#ifndef ATTACKS_HPP
#define ATTACKS_HPP

#include "board/types.hpp"
#include "board/bitboard.hpp"

// File masks
extern const U64 not_a_file;
extern const U64 not_h_file;
extern const U64 not_hg_file;
extern const U64 not_ab_file;

// Relevant bits for magic bitboards
extern const int bishop_relevant_bits[64];
extern const int rook_relevant_bits[64];

// Magic numbers (pre-computed)
extern U64 rook_magic_numbers[64];
extern U64 bishop_magic_numbers[64];

// Attack tables
extern U64 pawn_attacks[2][64];
extern U64 knight_attacks[64];
extern U64 king_attacks[64];
extern U64 bishop_masks[64];
extern U64 rook_masks[64];
extern U64 bishop_attacks[64][512];
extern U64 rook_attacks[64][4096];

// Attack mask generation functions
U64 mask_pawn_attacks(int side, int square);
U64 mask_knight_attacks(int square);
U64 mask_king_attacks(int square);
U64 mask_bishop_attacks(int square);
U64 mask_rook_attacks(int square);

// Attack generation on-the-fly (for magic initialization)
U64 bishop_attacks_on_the_fly(int square, U64 block);
U64 rook_attacks_on_the_fly(int square, U64 block);

// Set occupancy for magic bitboard initialization
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask);

// Get attacks using magic bitboards (inline for performance)
inline U64 get_bishop_attacks(int square, U64 occupancy) {
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];
    return bishop_attacks[square][occupancy];
}

inline U64 get_rook_attacks(int square, U64 occupancy) {
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];
    return rook_attacks[square][occupancy];
}

inline U64 get_queen_attacks(int square, U64 occupancy) {
    U64 queen_attacks = 0ULL;
    U64 bishop_occupancy = occupancy;
    U64 rook_occupancy = occupancy;
    
    bishop_occupancy &= bishop_masks[square];
    bishop_occupancy *= bishop_magic_numbers[square];
    bishop_occupancy >>= 64 - bishop_relevant_bits[square];
    queen_attacks = bishop_attacks[square][bishop_occupancy];
    
    rook_occupancy &= rook_masks[square];
    rook_occupancy *= rook_magic_numbers[square];
    rook_occupancy >>= 64 - rook_relevant_bits[square];
    queen_attacks |= rook_attacks[square][rook_occupancy];
    
    return queen_attacks;
}

// Initialize attack tables
void init_leapers_attacks();
void init_sliders_attacks(int bishop);

#endif // ATTACKS_HPP
