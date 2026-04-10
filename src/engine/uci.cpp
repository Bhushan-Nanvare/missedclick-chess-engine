#include "engine/uci.hpp"
#include "board/board.hpp"
#include "search/search.hpp"
#include "search/search_enhanced.hpp"
#include "moves/makemove.hpp"
#include "moves/movegen.hpp"
#include "search/moveorder.hpp"
#include "search/tt.hpp"
#include "utils/utils.hpp"
#include "eval/evaluation_enhanced.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace MissedClick {

static bool is_uci_command(const char* input, const char* command) {
    size_t n = strlen(command);
    if (strncmp(input, command, n) != 0) {
        return false;
    }
    char next = input[n];
    return next == '\0' || next == ' ' || next == '\t' || next == '\r' || next == '\n';
}

extern SearchInfo search_info;

int starttime = 0;
int stoptime = 0;
int timeset = 0;
int stopped = 0;
int quit = 0;
int movestogo = 30;
int time_left = -1;
int inc = 0;
int movetime = -1;
int depth = MAX_PLY;
int infinite = 0;

void uci_loop() {
    char command[4096];
    while (true) {
        memset(command, 0, sizeof(command));
        if (fgets(command, sizeof(command), stdin) == NULL)
            break;
        parse_uci_command(command);
    }
}

void parse_uci_command(const char* command) {
    while (*command == ' ' || *command == '\t' || *command == '\n' || *command == '\r')
        command++;

    if (is_uci_command(command, "uci")) {
        uci_id();
    }
    else if (is_uci_command(command, "setoption")) {
        // Accepted for GUI compatibility (no-op).
    }
    else if (is_uci_command(command, "isready")) {
        uci_isready();
    }
    else if (is_uci_command(command, "ucinewgame")) {
        uci_ucinewgame();
    }
    else if (is_uci_command(command, "position")) {
        uci_position(command);
    }
    else if (is_uci_command(command, "go")) {
        uci_go(command);
    }
    else if (is_uci_command(command, "stop")) {
        uci_stop();
    }
    else if (is_uci_command(command, "quit")) {
        uci_quit();
    }
    // Unknown commands are silently ignored per UCI spec
}

void uci_id() {
    printf("id name MissedClick 1.4\n");
    printf("id author MissedClick\n");
    uci_options();
    printf("uciok\n");
    fflush(stdout);
}

void uci_options() {
    printf("option name Hash type spin default 64 min 1 max 1024\n");
    printf("option name Threads type spin default 1 min 1 max 512\n");
    printf("option name Ponder type check default false\n");
}

void uci_isready() {
    printf("readyok\n");
    fflush(stdout);
}

void uci_ucinewgame() {
    reset_board();
    parse_fen(START_POSITION);
    repetition_index = 0;
    clear_tt();
    clear_move_ordering();
    g_evaluator_enhanced.clear_caches();
}

void uci_position(const char* command) {
    const char* position_cmd = command + 8;  // skip "position"
    while (*position_cmd == ' ' || *position_cmd == '\t')
        position_cmd++;

    // Reset repetition table for new position
    repetition_index = 0;

    if (strncmp(position_cmd, "startpos", 8) == 0) {
        parse_fen(START_POSITION);
        position_cmd += 8;
    }
    else if (strncmp(position_cmd, "fen", 3) == 0) {
        position_cmd += 3;
        while (*position_cmd == ' ' || *position_cmd == '\t')
            position_cmd++;

        const char* fen_end = strstr(position_cmd, " moves");
        if (fen_end == NULL)
            fen_end = strstr(position_cmd, "\tmoves");

        int fen_length;
        if (fen_end != NULL) {
            fen_length = (int)(fen_end - position_cmd);
        } else {
            fen_length = (int)strlen(position_cmd);
            // Strip trailing whitespace/newline
            while (fen_length > 0 && (position_cmd[fen_length-1] == '\n' ||
                   position_cmd[fen_length-1] == '\r' ||
                   position_cmd[fen_length-1] == ' '))
                fen_length--;
        }

        char fen_string[1024];
        if (fen_length >= (int)sizeof(fen_string))
            fen_length = (int)sizeof(fen_string) - 1;
        memcpy(fen_string, position_cmd, fen_length);
        fen_string[fen_length] = '\0';
        parse_fen(fen_string);

        position_cmd = fen_end;
    }
    else {
        // Unrecognised position type — default to start position
        parse_fen(START_POSITION);
    }

    // Find and parse moves list
    if (position_cmd != NULL) {
        const char* moves_start = strstr(position_cmd, "moves");
        if (moves_start != NULL) {
            const char* moves_ptr = moves_start + 5;  // skip "moves"

            while (*moves_ptr) {
                // Skip whitespace including newlines
                while (*moves_ptr == ' ' || *moves_ptr == '\t' ||
                       *moves_ptr == '\r' || *moves_ptr == '\n')
                    moves_ptr++;

                // End of string
                if (*moves_ptr == '\0')
                    break;

                // Read one move token
                char move_string[6];
                int move_length = 0;
                while (*moves_ptr && *moves_ptr != ' ' && *moves_ptr != '\t' &&
                       *moves_ptr != '\n' && *moves_ptr != '\r' && move_length < 5) {
                    move_string[move_length++] = *moves_ptr++;
                }
                move_string[move_length] = '\0';

                // Skip empty tokens (shouldn't happen after the whitespace skip)
                if (move_length == 0)
                    break;

                // Parse and make the move
                int move = parse_move(move_string);
                if (move != 0) {
                    // Save position in repetition table before making move
                    repetition_table[repetition_index++] = hash_key;
                    make_move(move, all_moves);
                }
            }
        }
    }
}

void uci_go(const char* command) {
    // Reset search info
    search_info.nodes = 0;
    search_info.stopped = 0;
    search_info.timeset = 0;
    search_info.starttime = get_time_ms();
    search_info.depth_limit = MAX_PLY;

    movestogo = 30;
    time_left = -1;
    inc = 0;
    movetime = -1;
    depth = MAX_PLY;
    infinite = 0;
    timeset = 0;

    char* cmd = strdup(command);
    char* token = strtok(cmd, " \t\r\n");
    token = strtok(NULL, " \t\r\n");  // skip "go"

    while (token != NULL) {
        if (strcmp(token, "wtime") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token != NULL && side == white)
                time_left = atoi(token);
        }
        else if (strcmp(token, "btime") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token != NULL && side == black)
                time_left = atoi(token);
        }
        else if (strcmp(token, "winc") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token != NULL && side == white)
                inc = atoi(token);
        }
        else if (strcmp(token, "binc") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token != NULL && side == black)
                inc = atoi(token);
        }
        else if (strcmp(token, "movestogo") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token != NULL)
                movestogo = atoi(token);
        }
        else if (strcmp(token, "depth") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token != NULL)
                depth = atoi(token);
        }
        else if (strcmp(token, "movetime") == 0) {
            token = strtok(NULL, " \t\r\n");
            if (token != NULL) {
                movetime = atoi(token);
                timeset = 1;
            }
        }
        else if (strcmp(token, "infinite") == 0) {
            infinite = 1;
        }
        token = strtok(NULL, " \t\r\n");
    }
    free(cmd);

    search_info.depth_limit = depth;

    if (movetime > 0) {
        enable_time_management(movetime);
        timeset = 1;
    } else if (!infinite && time_left > 0) {
        // Allocate time: a fraction of remaining time
        int allocated = time_left / movestogo + inc / 2;
        // Safety margin: never use more than half the remaining time
        if (allocated > time_left / 2) allocated = time_left / 2;
        if (allocated < 10) allocated = 10;
        enable_time_management(allocated);
        timeset = 1;
    }

    // Run search (best_move is tracked live inside negamax + iterative_deepening)
    search_position();

    int best_move = search_info.best_move;

    // Last-resort fallback: first legal move
    if (best_move == 0) {
        MoveList ml; generate_moves(&ml);
        for (int i = 0; i < ml.count; i++) {
            repetition_table[repetition_index++] = hash_key;
            if (make_move(ml.moves[i], all_moves)) {
                ply++;
                undo_move();        // ply-- inside
                repetition_index--;
                best_move = ml.moves[i];
                break;
            }
            repetition_index--;
        }
    }

    if (best_move != 0) {
        printf("bestmove ");
        print_move(best_move);
        printf("\n");
        fflush(stdout);
    } else {
        printf("bestmove 0000\n");
        fflush(stdout);
    }
}

void uci_stop() {
    stop_search();
}

void uci_quit() {
    exit(0);
}

int parse_move(const char* move_string) {
    if (!move_string || move_string[0] == '\0' || move_string[1] == '\0' ||
        move_string[2] == '\0' || move_string[3] == '\0')
        return 0;

    MoveList move_list;
    generate_moves(&move_list);

    int source_square = (move_string[0] - 'a') + (7 - (move_string[1] - '1')) * 8;
    int target_square = (move_string[2] - 'a') + (7 - (move_string[3] - '1')) * 8;

    for (int count = 0; count < move_list.count; count++) {
        int move = move_list.moves[count];

        if (get_move_source(move) == source_square && get_move_target(move) == target_square) {
            int promoted_piece = get_move_promoted(move);

            if (move_string[4] != '\0' && move_string[4] != '\n' && move_string[4] != '\r') {
                // Promotion move
                int expected = 0;
                switch (move_string[4]) {
                    case 'n': case 'N': expected = (side == white) ? N : n; break;
                    case 'b': case 'B': expected = (side == white) ? B : b; break;
                    case 'r': case 'R': expected = (side == white) ? R : r; break;
                    case 'q': case 'Q': expected = (side == white) ? Q : q; break;
                }
                if (promoted_piece == expected)
                    return move;
            } else {
                if (promoted_piece == 0)
                    return move;
            }
        }
    }

    return 0;
}

void print_move(int move) {
    int source_square = get_move_source(move);
    int target_square = get_move_target(move);

    printf("%c%c", 'a' + (source_square % 8), '1' + (7 - source_square / 8));
    printf("%c%c", 'a' + (target_square % 8), '1' + (7 - target_square / 8));

    int promoted_piece = get_move_promoted(move);
    if (promoted_piece) {
        // Always output lowercase promotion piece per UCI spec
        switch (promoted_piece) {
            case N: case n: printf("n"); break;
            case B: case b: printf("b"); break;
            case R: case r: printf("r"); break;
            case Q: case q: printf("q"); break;
        }
    }
}

} // namespace MissedClick
