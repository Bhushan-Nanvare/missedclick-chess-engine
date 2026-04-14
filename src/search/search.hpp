#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "board/types.hpp"
#include "moves/movegen.hpp"

namespace MissedClick {

// UCI-facing search control and stats; best_move is filled while searching the root.
struct SearchInfo {
    int  starttime;
    int  stoptime;
    int  stopped;
    int  quit;
    int  timeset;
    int  depth_limit;
    int  nodes;
    int  fh;
    int  fhf;
    int  best_move;   // Best move found at root — updated inside negamax
};

void search_position();
int  negamax(int alpha, int beta, int depth);
int  quiescence(int alpha, int beta);
int  pvs(int alpha, int beta, int depth);
int  lmr(int depth, int moves_searched);
int  nmp(int beta, int depth);
void iterative_deepening();
void enable_time_management(int time_ms);
bool is_time_up();
void stop_search();
void print_thinking(int depth, int score, int nodes, int time, MoveList* pv);
void get_pv_line(int depth, MoveList* pv);
bool is_repetition();
void update_search_stats(int move, int depth);

} // namespace MissedClick

#endif // SEARCH_HPP
