#ifndef UCI_HPP
#define UCI_HPP

#include "board/types.hpp"

namespace MissedClick {

// UCI loop
void uci_loop();

// Parse UCI command
void parse_uci_command(const char* command);

// UCI identification
void uci_id();

// UCI options
void uci_options();

// UCI ready
void uci_isready();

// UCI new game
void uci_ucinewgame();

// UCI position command
void uci_position(const char* command);

// UCI go command
void uci_go(const char* command);

// UCI stop command
void uci_stop();

// UCI quit command
void uci_quit();

// Parse move from string
int parse_move(const char* move_string);

// Print move in UCI format
void print_move(int move);

} // namespace MissedClick

#endif // UCI_HPP
