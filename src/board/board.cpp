#include "board/board.hpp"
#include "board/hash.hpp"
#include "board/bitboard.hpp"
#include "board/types.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Bitmask of rights to keep when a piece leaves this square (15 = keep all four bits).
const int castling_rights[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

namespace MissedClick {
    U64 bitboards[12] = {0};
    U64 occupancies[3] = {0};
    int side = 0;
    int enpassant = no_sq;
    int castle = 0;
    U64 hash_key = 0;
    U64 repetition_table[1000] = {0};
    int repetition_index = 0;
    int ply = 0;
    int fifty = 0;
    
    // Board state backup for move making
    BoardState board_state;
    
    // Reset board to empty position
    void reset_board() {
        std::memset(MissedClick::bitboards, 0, sizeof(MissedClick::bitboards));
        std::memset(MissedClick::occupancies, 0, sizeof(MissedClick::occupancies));
        MissedClick::side = 0;
        MissedClick::enpassant = no_sq;
        MissedClick::castle = 0;
        MissedClick::repetition_index = 0;
        MissedClick::fifty = 0;
        std::memset(MissedClick::repetition_table, 0, sizeof(MissedClick::repetition_table));
        MissedClick::ply = 0;
        MissedClick::hash_key = 0ULL;
    }
    
    // Print bitboard (for debugging)
    void print_bitboard(U64 bitboard) {
        printf("\n");
        
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                
                if (!file)
                    printf("  %d ", 8 - rank);
                
                printf(" %d", GET_BIT(bitboard, square) ? 1 : 0);
            }
            printf("\n");
        }
        
        printf("\n     a b c d e f g h\n\n");
        printf("     Bitboard: %llu\n\n", bitboard);
    }
    
    // Print board (for debugging)
    void print_board() {
        printf("\n");
        
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                
                if (!file)
                    printf("  %d ", 8 - rank);
                
                int piece = -1;
                for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                    if (GET_BIT(MissedClick::bitboards[bb_piece], square))
                        piece = bb_piece;
                }
                
#ifdef WIN64
                printf(" %c", (piece == -1) ? '.' : ASCII_PIECES[piece]);
#else
                printf(" %s", (piece == -1) ? "." : UNICODE_PIECES[piece]);
#endif
            }
            printf("\n");
        }
        
        printf("\n     a b c d e f g h\n\n");
        printf("     Side:     %s\n", !MissedClick::side ? "white" : "black");
        printf("     Enpassant:   %s\n", (MissedClick::enpassant != no_sq) ? SQUARE_TO_COORDINATES[MissedClick::enpassant] : "no");
        printf("     Castling:  %c%c%c%c\n\n", 
               (MissedClick::castle & wk) ? 'K' : '-',
               (MissedClick::castle & wq) ? 'Q' : '-',
               (MissedClick::castle & bk) ? 'k' : '-',
               (MissedClick::castle & bq) ? 'q' : '-');
        printf("     Hash key:  %llx\n", MissedClick::hash_key);
        printf("     Fifty move: %d\n\n", MissedClick::fifty);
    }
    
    // Parse FEN string
    void parse_fen(const char* fen) {
        reset_board();
        
        // Loop over board ranks
        for (int rank = 0; rank < 8; rank++) {
            // Loop over board files
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                
                // Match ASCII pieces within FEN string
                if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
                    int piece = CHAR_PIECES[*fen];
                    SET_BIT(MissedClick::bitboards[piece], square);
                    fen++;
                }
                
                // Match empty square numbers within FEN string
                if (*fen >= '0' && *fen <= '9') {
                    int offset = *fen - '0';
                    int piece = -1;
                    
                    for (int bb_piece = P; bb_piece <= k; bb_piece++) {
                        if (GET_BIT(MissedClick::bitboards[bb_piece], square))
                            piece = bb_piece;
                    }
                    
                    if (piece == -1)
                        file--;
                    
                    file += offset;
                    fen++;
                }
                
                // Match rank separator
                if (*fen == '/')
                    fen++;
            }
        }
        
        // Go to parsing side to move
        fen++;
        
        // Parse side to move
        MissedClick::side = (*fen == 'w') ? white : black;
        
        // Go to parsing castling rights
        fen += 2;
        
        // Parse castling rights
        while (*fen != ' ') {
            switch (*fen) {
                case 'K': MissedClick::castle |= wk; break;
                case 'Q': MissedClick::castle |= wq; break;
                case 'k': MissedClick::castle |= bk; break;
                case 'q': MissedClick::castle |= bq; break;
                case '-': break;
            }
            fen++;
        }
        
        // Go to parsing enpassant square
        fen++;
        
        // Parse enpassant square
        if (*fen != '-') {
            int file = fen[0] - 'a';
            int rank = 8 - (fen[1] - '0');
            MissedClick::enpassant = rank * 8 + file;
        } else {
            MissedClick::enpassant = no_sq;
        }
        
        // Go to parsing half move counter
        fen++;
        
        // Parse half move counter to init fifty move counter
        MissedClick::fifty = atoi(fen);
        
        // Rebuild aggregate masks from piece boards (needed for sliders and occupancy tests).
        for (int piece = P; piece <= K; piece++)
            MissedClick::occupancies[white] |= MissedClick::bitboards[piece];
        
        for (int piece = p; piece <= k; piece++)
            MissedClick::occupancies[black] |= MissedClick::bitboards[piece];
        
        MissedClick::occupancies[both] |= MissedClick::occupancies[white];
        MissedClick::occupancies[both] |= MissedClick::occupancies[black];
        
        // Init hash key
        MissedClick::hash_key = generate_hash_key();
    }
} // namespace MissedClick
