#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

// Bitboard type
using U64 = uint64_t;

// Square index: rank 0 = 8th rank (a8..h8), rank 7 = 1st rank (a1..h1). Matches FEN top-to-bottom.
enum Square {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};

// Piece encoding
enum Piece {
    P, N, B, R, Q, K,  // White pieces
    p, n, b, r, q, k   // Black pieces
};

// Side to move (colors)
enum Side {
    white, black, both
};

// Slider piece types
enum SliderType {
    rook, bishop
};

// Castling rights
enum CastlingRights {
    wk = 1,  // White king side
    wq = 2,  // White queen side
    bk = 4,  // Black king side
    bq = 8   // Black queen side
};

// Move types
enum MoveType {
    all_moves, only_captures
};

// Transposition table hash flags
enum HashFlag {
    hash_flag_exact = 0,
    hash_flag_alpha = 1,
    hash_flag_beta = 2
};

// Constants
constexpr const char* VERSION = " - FINAL VERSION (1.4 + SF NNUE)";

constexpr const char* EMPTY_BOARD = "8/8/8/8/8/8/8/8 b - - ";
constexpr const char* START_POSITION = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
constexpr const char* TRICKY_POSITION = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ";
constexpr const char* KILLER_POSITION = "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1";
constexpr const char* CMK_POSITION = "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 ";
constexpr const char* REPETITIONS = "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 ";

// Search constants
constexpr int INFINITY = 50000;
constexpr int MATE_VALUE = 49000;
constexpr int MATE_SCORE = 48000;
constexpr int MAX_PLY = 64;
constexpr int NO_HASH_ENTRY = 100000;

// Search parameters
constexpr int FULL_DEPTH_MOVES = 4;
constexpr int REDUCTION_LIMIT = 3;

// Square to coordinates conversion
extern const char* SQUARE_TO_COORDINATES[];

// ASCII pieces
extern const char ASCII_PIECES[];

// Unicode pieces (for non-Windows)
extern const char* UNICODE_PIECES[];

// Character to piece conversion
extern int CHAR_PIECES[];

// Promoted pieces
extern char PROMOTED_PIECES[];

// Move encoding macros
// binary move bits                               hexidecimal constants
// 0000 0000 0000 0000 0011 1111    source square       0x3f
// 0000 0000 0000 1111 1100 0000    target square       0xfc0
// 0000 0000 1111 0000 0000 0000    piece               0xf000
// 0000 1111 0000 0000 0000 0000    promoted piece      0xf0000
// 0001 0000 0000 0000 0000 0000    capture flag        0x100000
// 0010 0000 0000 0000 0000 0000    double push flag    0x200000
// 0100 0000 0000 0000 0000 0000    enpassant flag      0x400000
// 1000 0000 0000 0000 0000 0000    castling flag       0x800000

#define ENCODE_MOVE(source, target, piece, promoted, capture, double_push, enpassant, castling) \
    ((source) |          \
    ((target) << 6) |     \
    ((piece) << 12) |     \
    ((promoted) << 16) |  \
    ((capture) << 20) |   \
    ((double_push) << 21) |    \
    ((enpassant) << 22) | \
    ((castling) << 23))

#define GET_MOVE_SOURCE(move) ((move) & 0x3f)
#define GET_MOVE_TARGET(move) (((move) & 0xfc0) >> 6)
#define GET_MOVE_PIECE(move) (((move) & 0xf000) >> 12)
#define GET_MOVE_PROMOTED(move) (((move) & 0xf0000) >> 16)
#define GET_MOVE_CAPTURE(move) ((move) & 0x100000)
#define GET_MOVE_DOUBLE(move) ((move) & 0x200000)
#define GET_MOVE_ENPASSANT(move) ((move) & 0x400000)
#define GET_MOVE_CASTLING(move) ((move) & 0x800000)

// Move encoding/decoding functions
inline int get_move_source(int move) { return move & 0x3f; }
inline int get_move_target(int move) { return (move & 0xfc0) >> 6; }
inline int get_move_piece(int move) { return (move & 0xf000) >> 12; }
inline int get_move_promoted(int move) { return (move & 0xf0000) >> 16; }
inline int get_move_capture(int move) { return move & 0x100000; }
inline int get_move_double(int move) { return move & 0x200000; }
inline int get_move_enpassant(int move) { return move & 0x400000; }
inline int get_move_castling(int move) { return move & 0x800000; }

// Move list structure
struct MoveList {
    int moves[256];
    int scores[256];
    int count;
};

#endif // TYPES_HPP
