#include "search/moveorder.hpp"
#include "board/board.hpp"
#include "board/bitboard.hpp"
#include "search/tt.hpp"
#include "board/hash.hpp"
#include <cstring>

namespace MissedClick {

// Transposition table
extern TTEntry tt[TT_SIZE];

// MVV-LVA (Most Valuable Victim - Least Valuable Attacker) scores
const int mvv_lva_scores[12][12] = {
    // Victim: P, N, B, R, Q, K, p, n, b, r, q, k
    {105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605}, // P (White Pawn)
    {104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604}, // N (White Knight)
    {103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603}, // B (White Bishop)
    {102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602}, // R (White Rook)
    {101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601}, // Q (White Queen)
    {100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600}, // K (White King)
    {105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605}, // p (Black Pawn)
    {104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604}, // n (Black Knight)
    {103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603}, // b (Black Bishop)
    {102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602}, // r (Black Rook)
    {101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601}, // q (Black Queen)
    {100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600}  // k (Black King)
};

// Killer moves (2 per ply)
int killer_moves[2][600];

// History heuristic scores
int history_scores[12][64];

// Clear killer moves and history scores
void clear_move_ordering() {
    std::memset(killer_moves, 0, sizeof(killer_moves));
    std::memset(history_scores, 0, sizeof(history_scores));
}

// Update killer moves
void update_killer_moves(int ply, int move) {
    // Don't store capture moves as killers
    if (get_move_capture(move))
        return;
    
    // Move existing killer move to second position
    killer_moves[1][ply] = killer_moves[0][ply];
    
    // Store new move as first killer
    killer_moves[0][ply] = move;
}

// Update history scores
void update_history_scores(int move, int depth) {
    int piece = get_move_piece(move);
    int target_square = get_move_target(move);
    
    // Increment history score based on depth
    history_scores[piece][target_square] += depth * depth;
}

// Score move for ordering
int score_move(int move) {
    int score = 0;
    
    // Get PV move from transposition table
    int pv_move = get_pv_move();
    
    // PV move gets highest score
    if (move == pv_move)
        score += 2000000;
    
    // Check for captures (get_move_capture returns a FLAG, not a piece index!)
    else if (get_move_capture(move)) {
        int moving_piece = get_move_piece(move);
        int target_sq   = get_move_target(move);
        
        // Find the actual captured piece by scanning opponent bitboards
        int victim = 0; // default pawn value if nothing found (enpassant)
        int start_piece = (side == white) ? p : P;
        int end_piece   = (side == white) ? k : K;
        for (int pp = start_piece; pp <= end_piece; pp++) {
            if (GET_BIT(bitboards[pp], target_sq)) {
                victim = pp;
                break;
            }
        }
        // MVV-LVA: rows=attacker, cols=victim -> high victim + low attacker = high score
        score += mvv_lva_scores[moving_piece][victim] + 1000000;
    }
    
    // Check for promotions
    else if (get_move_promoted(move)) {
        score += 900000;
    }
    
    // Check for first killer move
    else if (move == killer_moves[0][ply]) {
        score += 800000;
    }
    
    // Check for second killer move
    else if (move == killer_moves[1][ply]) {
        score += 700000;
    }
    
    // Add history score
    int piece = get_move_piece(move);
    int target_square = get_move_target(move);
    score += history_scores[piece][target_square];
    
    return score;
}

// Sort moves by score (insertion sort for efficiency)
void sort_moves(MoveList* move_list) {
    // Score all moves
    for (int count = 0; count < move_list->count; count++) {
        move_list->scores[count] = MissedClick::score_move(move_list->moves[count]);
    }
    
    // Insertion sort
    for (int current = 1; current < move_list->count; current++) {
        int temp_move = move_list->moves[current];
        int temp_score = move_list->scores[current];
        int i = current - 1;
        
        while (i >= 0 && move_list->scores[i] < temp_score) {
            move_list->moves[i + 1] = move_list->moves[i];
            move_list->scores[i + 1] = move_list->scores[i];
            i--;
        }
        
        move_list->moves[i + 1] = temp_move;
        move_list->scores[i + 1] = temp_score;
    }
}

// Get principal variation move from transposition table
int get_pv_move() {
    // Get current position hash key
    U64 key = hash_key;
    
    // Get index from hash key
    int index = key % TT_SIZE;
    
    // Return PV move if key matches
    if (tt[index].key == key)
        return tt[index].move;
    
    return 0;
}

} // namespace MissedClick
