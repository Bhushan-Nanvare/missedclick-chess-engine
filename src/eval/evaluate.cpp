// Material plus piece-square tables (white-centric tables; black uses mirrored square index).
#include "eval/evaluate.hpp"
#include "board/board.hpp"
#include "board/bitboard.hpp"
#include "attacks/attacks.hpp"
#include "moves/movegen.hpp"
#include <cstdio>

namespace MissedClick {

// Material values (for quick checks)
static const int piece_values[12] = {
    100, 325, 335, 500, 975, 20000,   // P N B R Q K (white)
    100, 325, 335, 500, 975, 20000    // p n b r q k (black)
};

static const int pawn_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

static const int knight_pst[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

static const int bishop_pst[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

static const int rook_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

static const int queen_pst[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

static const int king_mg_pst[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

static int evaluate_absolute() {
    int score = 0;

    for (int piece = P; piece <= K; piece++) {
        U64 bb = bitboards[piece];
        while (bb) {
            int sq = get_ls1b_index(bb);
            score += piece_values[piece];

            switch (piece) {
                case P: score += pawn_pst[sq];       break;
                case N: score += knight_pst[sq];     break;
                case B: score += bishop_pst[sq];     break;
                case R: score += rook_pst[sq];       break;
                case Q: score += queen_pst[sq];      break;
                case K: score += king_mg_pst[sq];    break;
            }
            POP_BIT(bb, sq);
        }
    }

    // Black pieces (mirror square index for PST)
    for (int piece = p; piece <= k; piece++) {
        U64 bb = bitboards[piece];
        while (bb) {
            int sq = get_ls1b_index(bb);
            score -= piece_values[piece - 6];

            switch (piece) {
                case p: score -= pawn_pst[63 - sq];    break;
                case n: score -= knight_pst[63 - sq];  break;
                case b: score -= bishop_pst[63 - sq];  break;
                case r: score -= rook_pst[63 - sq];    break;
                case q: score -= queen_pst[63 - sq];   break;
                case k: score -= king_mg_pst[63 - sq]; break;
            }
            POP_BIT(bb, sq);
        }
    }

    if (count_bits(bitboards[B]) >= 2) score += 50;
    if (count_bits(bitboards[b]) >= 2) score -= 50;

    return score;
}

int evaluate() {
    if (fifty >= 100) return 0;

    if (!bitboards[K]) return -MATE_VALUE;
    if (!bitboards[k]) return  MATE_VALUE;

    int score = evaluate_absolute(); // + favors white

    return (side == white) ? score : -score;
}

// Check if position is a draw
bool is_draw() {
    if (fifty >= 100) return true;

    int repetitions = 0;
    for (int i = repetition_index - 1; i >= 0; i--) {
        if (repetition_table[i] == hash_key) {
            repetitions++;
            if (repetitions >= 2) return true;
        }
    }

    // Insufficient material: K vs K
    if (!bitboards[P] && !bitboards[N] && !bitboards[B] &&
        !bitboards[R] && !bitboards[Q] &&
        !bitboards[p] && !bitboards[n] && !bitboards[b] &&
        !bitboards[r] && !bitboards[q])
        return true;

    // K+B vs K or K+N vs K
    int white_minor = count_bits(bitboards[N]) + count_bits(bitboards[B]);
    int black_minor = count_bits(bitboards[n]) + count_bits(bitboards[b]);
    if (!bitboards[P] && !bitboards[R] && !bitboards[Q] &&
        !bitboards[p] && !bitboards[r] && !bitboards[q] &&
        white_minor <= 1 && black_minor <= 1)
        return true;

    return false;
}

// Check if position is a checkmate
bool is_checkmate() {
    MoveList move_list;
    generate_moves(&move_list);
    if (move_list.count == 0) {
        int king_square = (side == white) ? get_ls1b_index(bitboards[K]) : get_ls1b_index(bitboards[k]);
        return is_square_attacked(king_square, side ^ 1);
    }
    return false;
}

// Check if position is a stalemate
bool is_stalemate() {
    MoveList move_list;
    generate_moves(&move_list);
    if (move_list.count == 0) {
        int king_square = (side == white) ? get_ls1b_index(bitboards[K]) : get_ls1b_index(bitboards[k]);
        return !is_square_attacked(king_square, side ^ 1);
    }
    return false;
}

// Get material balance
int get_material_balance() {
    int balance = 0;
    for (int piece = P; piece <= K; piece++)
        balance += piece_values[piece] * count_bits(bitboards[piece]);
    for (int piece = p; piece <= k; piece++)
        balance -= piece_values[piece - 6] * count_bits(bitboards[piece]);
    return balance;
}

int get_piece_square_score() { return 0; }
int evaluate_endgame()        { return evaluate(); }
int evaluate_opening()        { return evaluate(); }
int evaluate_middlegame()     { return evaluate(); }

// nnue_refresh is declared in evaluate.hpp – provide empty body
void nnue_refresh() {}

} // namespace MissedClick
