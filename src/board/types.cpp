#include "board/types.hpp"

// Square to coordinates conversion
const char* SQUARE_TO_COORDINATES[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

// ASCII pieces
const char ASCII_PIECES[] = "PNBRQKpnbrqk";

// Unicode pieces (for non-Windows)
const char* UNICODE_PIECES[] = {"♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};

// Character to piece conversion
int CHAR_PIECES[256] = {0};

// Initialize CHAR_PIECES array
struct CharPiecesInit {
    CharPiecesInit() {
        CHAR_PIECES['P'] = P;
        CHAR_PIECES['N'] = N;
        CHAR_PIECES['B'] = B;
        CHAR_PIECES['R'] = R;
        CHAR_PIECES['Q'] = Q;
        CHAR_PIECES['K'] = K;
        CHAR_PIECES['p'] = p;
        CHAR_PIECES['n'] = n;
        CHAR_PIECES['b'] = b;
        CHAR_PIECES['r'] = r;
        CHAR_PIECES['q'] = q;
        CHAR_PIECES['k'] = k;
    }
};

static CharPiecesInit char_pieces_init;

// Promoted pieces
char PROMOTED_PIECES[12] = {0};

// Initialize PROMOTED_PIECES array
struct PromotedPiecesInit {
    PromotedPiecesInit() {
        PROMOTED_PIECES[Q] = 'q';
        PROMOTED_PIECES[R] = 'r';
        PROMOTED_PIECES[B] = 'b';
        PROMOTED_PIECES[N] = 'n';
        PROMOTED_PIECES[q] = 'q';
        PROMOTED_PIECES[r] = 'r';
        PROMOTED_PIECES[b] = 'b';
        PROMOTED_PIECES[n] = 'n';
    }
};

static PromotedPiecesInit promoted_pieces_init;
