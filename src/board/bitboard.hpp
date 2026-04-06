#ifndef BITBOARD_HPP
#define BITBOARD_HPP

#include "board/types.hpp"
#include "board/bitboard_optimized.hpp"

// Each U64 is one bit per square (Square enum bit index). SET/GET/POP are the usual read-modify ops.

// Bit manipulation macros (keeping for compatibility)
#define SET_BIT(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define GET_BIT(bitboard, square) ((bitboard) & (1ULL << (square)))
#define POP_BIT(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// Compatibility helpers (some modules use these names)
inline void set_bit(U64& bitboard, int square) { SET_BIT(bitboard, square); }
inline void pop_bit(U64& bitboard, int square) { POP_BIT(bitboard, square); }

// Legacy functions (for compatibility)
inline int count_bits(U64 bitboard) {
    return count_bits_optimized(bitboard);
}

inline int get_ls1b_index(U64 bitboard) {
    return get_ls1b_index_optimized(bitboard);
}

// New optimized functions
inline int get_ms1b_index(U64 bitboard) {
    return get_ms1b_index_optimized(bitboard);
}

inline int extract_bit_squares(U64 bitboard, int* squares, int max_squares) {
    return extract_bits(bitboard, squares, max_squares);
}

#endif // BITBOARD_HPP
