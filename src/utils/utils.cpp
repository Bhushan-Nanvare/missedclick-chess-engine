#include "utils/utils.hpp"
#include "board/types.hpp"
#include "search/search.hpp"
#include <cstring>
#include <cstdlib>

// Accept WIN64 (custom define), _WIN64 (MinGW 64-bit), or _WIN32 (MinGW 32-bit)
#if defined(WIN64) || defined(_WIN64) || defined(_WIN32)
    #define ON_WINDOWS 1
    #include <windows.h>
    #include <io.h>
    #define read   _read
    #define fileno _fileno
#else
    #define ON_WINDOWS 0
    #include <sys/time.h>
    #include <unistd.h>
    #include <sys/select.h>
#endif

namespace MissedClick {
    extern int quit;
    extern SearchInfo search_info;
}

// Pseudo random number state
static unsigned int random_state = 1804289383;

// Generate 32-bit pseudo random numbers
unsigned int get_random_U32_number() {
    unsigned int number = random_state;
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;
    random_state = number;
    return number;
}

// Generate 64-bit pseudo random numbers
U64 get_random_U64_number() {
    U64 n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    U64 n2 = (U64)(get_random_U32_number()) & 0xFFFF;
    U64 n3 = (U64)(get_random_U32_number()) & 0xFFFF;
    U64 n4 = (U64)(get_random_U32_number()) & 0xFFFF;
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

// Generate magic number candidate
U64 generate_magic_number() {
    return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}

// Get time in milliseconds
int get_time_ms() {
#if ON_WINDOWS
    return (int)GetTickCount();
#else
    struct timeval time_value;
    gettimeofday(&time_value, nullptr);
    return (int)(time_value.tv_sec * 1000 + time_value.tv_usec / 1000);
#endif
}

// Check if input is waiting (platform-specific)
int input_waiting() {
#if ON_WINDOWS
    static int init = 0, pipe;
    static HANDLE inh;
    DWORD dw;

    if (!init) {
        init = 1;
        inh = GetStdHandle(STD_INPUT_HANDLE);
        pipe = !GetConsoleMode(inh, &dw);
        if (!pipe) {
            SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(inh);
        }
    }
    
    if (pipe) {
        if (!PeekNamedPipe(inh, nullptr, 0, nullptr, &dw, nullptr)) return 1;
        return dw;
    } else {
        GetNumberOfConsoleInputEvents(inh, &dw);
        return dw <= 1 ? 0 : dw;
    }
#else
    fd_set readfds;
    struct timeval tv;
    FD_ZERO(&readfds);
    FD_SET(fileno(stdin), &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    select(16, &readfds, 0, 0, &tv);
    return (FD_ISSET(fileno(stdin), &readfds));
#endif
}

// Read GUI/user input (non-blocking peek during search)
void read_input() {
    if (!input_waiting())
        return;

    char input[256];
    int bytes = read(fileno(stdin), input, (int)sizeof(input) - 1);
    if (bytes <= 0)
        return;
    input[bytes] = '\0';

    char* endc = strchr(input, '\n');
    if (endc) *endc = '\0';

    const char* p = input;
    while (*p == ' ' || *p == '\t' || *p == '\r') p++;

    if (!strncmp(p, "quit", 4)) {
        MissedClick::quit = 1;
        MissedClick::search_info.stopped = 1;
    } else if (!strncmp(p, "stop", 4)) {
        MissedClick::search_info.stopped = 1;
    }
}

// Bridge function to interact between search and GUI input
void communicate() {
    if (MissedClick::search_info.timeset && get_time_ms() >= MissedClick::search_info.stoptime)
        MissedClick::search_info.stopped = 1;
    read_input();
}
