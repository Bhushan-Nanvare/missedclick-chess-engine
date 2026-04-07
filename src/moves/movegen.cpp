#include "moves/movegen.hpp"
#include "board/types.hpp"
#include "board/bitboard.hpp"
#include "attacks/attacks.hpp"
#include "board/board.hpp"
#include <cstdio>

using namespace MissedClick;

namespace MissedClick {

// "side" is the attacker color. Sliders use the full board (both colors) as blockers.
bool is_square_attacked(int square, int side) {
    // Attacked by white pawns
    if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) return true;
    
    // Attacked by black pawns
    if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) return true;
    
    // Attacked by knights
    if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return true;
    
    // Attacked by bishops
    if (get_bishop_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return true;

    // Attacked by rooks
    if (get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return true;    

    // Attacked by queens
    if (get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return true;
    
    // Attacked by kings
    if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return true;

    // By default return false
    return false;
}

void generate_moves(MoveList* move_list) {
    move_list->count = 0;

    int source_square, target_square;
    U64 bitboard, attacks;

    // Walk every piece type; each branch only emits moves for the side to move and matching type.
    for (int piece = P; piece <= k; piece++) {
        // Init piece bitboard copy
        bitboard = bitboards[piece];
        
        // Generate white pawns & white king castling moves
        if (side == white) {
            // Pick up white pawn bitboards index
            if (piece == P) {
                // Loop over white pawns within white pawn bitboard
                while (bitboard) {
                    // Init source square
                    source_square = get_ls1b_index(bitboard);
                    
                    // Init target square
                    target_square = source_square - 8;
                    
                    // Generate quiet pawn moves
                    if (!(target_square < a8) && !GET_BIT(occupancies[both], target_square)) {
                        // Pawn promotion
                        if (source_square >= a7 && source_square <= h7) {
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, R, 0, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, B, 0, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, N, 0, 0, 0, 0));
                        } else {
                            // One square ahead pawn move
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            
                            // Two squares ahead pawn move
                            if ((source_square >= a2 && source_square <= h2) && !GET_BIT(occupancies[both], target_square - 8))
                                add_move(move_list, ENCODE_MOVE(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                        }
                    }
                    
                    // Init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[black];
                    
                    // Generate pawn captures
                    while (attacks) {
                        // Init target square
                        target_square = get_ls1b_index(attacks);
                        
                        // Pawn promotion
                        if (source_square >= a7 && source_square <= h7) {
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, N, 1, 0, 0, 0));
                        } else {
                            // One square ahead pawn move
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        
                        // Pop ls1b of the pawn attacks
                        POP_BIT(attacks, target_square);
                    }
                    
                    // Generate enpassant captures
                    if (enpassant != no_sq) {
                        // Lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        
                        // Make sure enpassant capture available
                        if (enpassant_attacks) {
                            // Init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, ENCODE_MOVE(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    
                    // Pop ls1b from piece bitboard copy
                    POP_BIT(bitboard, source_square);
                }
            }
            
            if (piece == K) {
                // Castling: empty transit squares; king may not pass through check.
                if (castle & wk) {
                    if (!GET_BIT(occupancies[both], f1) && !GET_BIT(occupancies[both], g1)) {
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                            add_move(move_list, ENCODE_MOVE(e1, g1, piece, 0, 0, 0, 0, 1));
                    }
                }
                
                if (castle & wq) {
                    if (!GET_BIT(occupancies[both], d1) && !GET_BIT(occupancies[both], c1) && !GET_BIT(occupancies[both], b1)) {
                        if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))
                            add_move(move_list, ENCODE_MOVE(e1, c1, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }
        
        // Generate black pawns & black king castling moves
        else {
            // Pick up black pawn bitboards index
            if (piece == p) {
                // Loop over black pawns within black pawn bitboard
                while (bitboard) {
                    // Init source square
                    source_square = get_ls1b_index(bitboard);
                    
                    // Init target square
                    target_square = source_square + 8;
                    
                    // Generate quiet pawn moves
                    if (!(target_square > h1) && !GET_BIT(occupancies[both], target_square)) {
                        // Pawn promotion
                        if (source_square >= a2 && source_square <= h2) {
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, n, 0, 0, 0, 0));
                        } else {
                            // One square ahead pawn move
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            
                            // Two squares ahead pawn move
                            if ((source_square >= a7 && source_square <= h7) && !GET_BIT(occupancies[both], target_square + 8))
                                add_move(move_list, ENCODE_MOVE(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                        }
                    }
                    
                    // Init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[white];
                    
                    // Generate pawn captures
                    while (attacks) {
                        // Init target square
                        target_square = get_ls1b_index(attacks);
                        
                        // Pawn promotion
                        if (source_square >= a2 && source_square <= h2) {
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, n, 1, 0, 0, 0));
                        } else {
                            // One square ahead pawn move
                            add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        
                        // Pop ls1b of the pawn attacks
                        POP_BIT(attacks, target_square);
                    }
                    
                    // Generate enpassant captures
                    if (enpassant != no_sq) {
                        // Lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        
                        // Make sure enpassant capture available
                        if (enpassant_attacks) {
                            // Init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, ENCODE_MOVE(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    
                    // Pop ls1b from piece bitboard copy
                    POP_BIT(bitboard, source_square);
                }
            }
            
            if (piece == k) {
                if (castle & bk) {
                    if (!GET_BIT(occupancies[both], f8) && !GET_BIT(occupancies[both], g8)) {
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                            add_move(move_list, ENCODE_MOVE(e8, g8, piece, 0, 0, 0, 0, 1));
                    }
                }
                
                if (castle & bq) {
                    if (!GET_BIT(occupancies[both], d8) && !GET_BIT(occupancies[both], c8) && !GET_BIT(occupancies[both], b8)) {
                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white))
                            add_move(move_list, ENCODE_MOVE(e8, c8, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }
        
        // Generate knight moves
        if ((side == white) ? piece == N : piece == n) {
            // Loop over source squares of piece bitboard copy
            while (bitboard) {
                // Init source square
                source_square = get_ls1b_index(bitboard);
                
                // Init piece attacks in order to get set of target squares
                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // Loop over target squares available from generated attacks
                while (attacks) {
                    // Init target square
                    target_square = get_ls1b_index(attacks);    
                    
                    // Quiet move
                    if (!GET_BIT(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        // Capture move
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    
                    // Pop ls1b in current attacks set
                    POP_BIT(attacks, target_square);
                }
                
                // Pop ls1b of the current piece bitboard copy
                POP_BIT(bitboard, source_square);
            }
        }
        
        // Generate bishop moves
        if ((side == white) ? piece == B : piece == b) {
            // Loop over source squares of piece bitboard copy
            while (bitboard) {
                // Init source square
                source_square = get_ls1b_index(bitboard);
                
                // Init piece attacks in order to get set of target squares
                attacks = get_bishop_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // Loop over target squares available from generated attacks
                while (attacks) {
                    // Init target square
                    target_square = get_ls1b_index(attacks);    
                    
                    // Quiet move
                    if (!GET_BIT(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        // Capture move
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    
                    // Pop ls1b in current attacks set
                    POP_BIT(attacks, target_square);
                }
                
                // Pop ls1b of the current piece bitboard copy
                POP_BIT(bitboard, source_square);
            }
        }
        
        // Generate rook moves
        if ((side == white) ? piece == R : piece == r) {
            // Loop over source squares of piece bitboard copy
            while (bitboard) {
                // Init source square
                source_square = get_ls1b_index(bitboard);
                
                // Init piece attacks in order to get set of target squares
                attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // Loop over target squares available from generated attacks
                while (attacks) {
                    // Init target square
                    target_square = get_ls1b_index(attacks);    
                    
                    // Quiet move
                    if (!GET_BIT(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        // Capture move
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    
                    // Pop ls1b in current attacks set
                    POP_BIT(attacks, target_square);
                }
                
                // Pop ls1b of the current piece bitboard copy
                POP_BIT(bitboard, source_square);
            }
        }
        
        // Generate queen moves
        if ((side == white) ? piece == Q : piece == q) {
            // Loop over source squares of piece bitboard copy
            while (bitboard) {
                // Init source square
                source_square = get_ls1b_index(bitboard);
                
                // Init piece attacks in order to get set of target squares
                attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // Loop over target squares available from generated attacks
                while (attacks) {
                    // Init target square
                    target_square = get_ls1b_index(attacks);    
                    
                    // Quiet move
                    if (!GET_BIT(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        // Capture move
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    
                    // Pop ls1b in current attacks set
                    POP_BIT(attacks, target_square);
                }
                
                // Pop ls1b of the current piece bitboard copy
                POP_BIT(bitboard, source_square);
            }
        }

        // Generate king moves
        if ((side == white) ? piece == K : piece == k) {
            // Loop over source squares of piece bitboard copy
            while (bitboard) {
                // Init source square
                source_square = get_ls1b_index(bitboard);
                
                // Init piece attacks in order to get set of target squares
                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                
                // Loop over target squares available from generated attacks
                while (attacks) {
                    // Init target square
                    target_square = get_ls1b_index(attacks);    
                    
                    // Quiet move
                    if (!GET_BIT(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        // Capture move
                        add_move(move_list, ENCODE_MOVE(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    
                    // Pop ls1b in current attacks set
                    POP_BIT(attacks, target_square);
                }

                // Pop ls1b of the current piece bitboard copy
                POP_BIT(bitboard, source_square);
            }
        }
    }
}

// Print move list
void print_move_list(MoveList* move_list) {
    // Do nothing on empty move list
    if (!move_list->count) {
        printf("\n     No move in the move list!\n");
        return;
    }
    
    printf("\n     move    piece     capture   double    enpass    castling\n\n");
    
    // Loop over moves within a move list
    for (int move_count = 0; move_count < move_list->count; move_count++) {
        // Init move
        int move = move_list->moves[move_count];
        
#ifdef WIN64
        // Print move
        printf("      %s%s%c   %c         %d         %d         %d         %d\n",
               SQUARE_TO_COORDINATES[GET_MOVE_SOURCE(move)],
               SQUARE_TO_COORDINATES[GET_MOVE_TARGET(move)],
               GET_MOVE_PROMOTED(move) ? PROMOTED_PIECES[GET_MOVE_PROMOTED(move)] : ' ',
               ASCII_PIECES[GET_MOVE_PIECE(move)],
               GET_MOVE_CAPTURE(move) ? 1 : 0,
               GET_MOVE_DOUBLE(move) ? 1 : 0,
               GET_MOVE_ENPASSANT(move) ? 1 : 0,
               GET_MOVE_CASTLING(move) ? 1 : 0);
#else
        // Print move
        printf("     %s%s%c   %s         %d         %d         %d         %d\n",
               SQUARE_TO_COORDINATES[GET_MOVE_SOURCE(move)],
               SQUARE_TO_COORDINATES[GET_MOVE_TARGET(move)],
               GET_MOVE_PROMOTED(move) ? PROMOTED_PIECES[GET_MOVE_PROMOTED(move)] : ' ',
               UNICODE_PIECES[GET_MOVE_PIECE(move)],
               GET_MOVE_CAPTURE(move) ? 1 : 0,
               GET_MOVE_DOUBLE(move) ? 1 : 0,
               GET_MOVE_ENPASSANT(move) ? 1 : 0,
               GET_MOVE_CASTLING(move) ? 1 : 0);
#endif
    }
    
    // Print total number of moves
    printf("\n\n     Total number of moves: %d\n\n", move_list->count);
}

} // namespace MissedClick
