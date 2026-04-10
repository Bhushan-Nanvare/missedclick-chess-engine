#include "board/board.hpp"
#include "attacks/attacks.hpp"
#include "engine/uci.hpp"
#include "utils/utils.hpp"
#include "board/hash.hpp"
#include "board/bitboard_optimized.hpp"
#include "eval/evaluation_enhanced.hpp"
#include "search/search_enhanced.hpp"
#include <cstdio>

using namespace MissedClick;

int main() {
    // Keep stdout line-buffered for stable UCI communication through GUIs.
    setvbuf(stdout, nullptr, _IOLBF, 0);
    
    // Initialize attack tables
    init_leapers_attacks();
    init_sliders_attacks(rook);
    init_sliders_attacks(bishop);
    
    // Initialize random number generator
    init_random_keys();
    
    // Initialize enhanced components
    g_evaluator_enhanced.clear_caches();
    g_search_enhanced.reset_stats();
    
    // Reset board to empty position
    reset_board();
    
    // Parse start position
    parse_fen(START_POSITION);
    
    // Start UCI loop
    uci_loop();
    
    return 0;
}
