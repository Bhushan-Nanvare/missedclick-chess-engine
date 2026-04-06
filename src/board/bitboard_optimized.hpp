#ifndef BITBOARD_OPTIMIZED_HPP
#define BITBOARD_OPTIMIZED_HPP

#include "board/types.hpp"

// Compiler-specific bit manipulation intrinsics (10-50x faster!)
#ifdef _MSC_VER  // MSVC
    #include <intrin.h>
    #define POPCOUNT(x) __popcnt64(x)
    #define LSB_INDEX(x) (x ? _tzcnt_u64(x) : 64)
    #define MSB_INDEX(x) (x ? 63 - _lzcnt_u64(x) : 64)
#elif __GNUC__  // GCC/Clang
    #define POPCOUNT(x) __builtin_popcountll(x)
    #define LSB_INDEX(x) (x ? __builtin_ctzll(x) : 64)
    #define MSB_INDEX(x) (x ? 63 - __builtin_clzll(x) : 64)
#endif

// Optimized bit count using hardware intrinsics
inline int count_bits_optimized(U64 bitboard) {
    return POPCOUNT(bitboard);  // Single CPU instruction!
}

// Optimized LSB index using hardware intrinsics
inline int get_ls1b_index_optimized(U64 bitboard) {
    return bitboard ? LSB_INDEX(bitboard) : -1;
}

// Optimized MSB index
inline int get_ms1b_index_optimized(U64 bitboard) {
    return bitboard ? MSB_INDEX(bitboard) : -1;
}

// Batch bit extraction for move generation
inline int extract_bits(U64 bitboard, int* squares, int max_squares) {
    int count = 0;
    while (bitboard && count < max_squares) {
        squares[count++] = LSB_INDEX(bitboard);
        bitboard &= bitboard - 1;  // Clear LSB
    }
    return count;
}

// Populated bitboard count (for move generation)
inline int popcount(U64 bitboard) {
    return POPCOUNT(bitboard);
}

#endif // BITBOARD_OPTIMIZED_HPP
