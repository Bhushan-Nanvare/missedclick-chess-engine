#ifndef UTILS_HPP
#define UTILS_HPP

#include "board/types.hpp"
#include <cstdio>

#ifdef WIN64
    #include <windows.h>
#else
    #include <sys/time.h>
    #include <unistd.h>
#endif

// Get time in milliseconds (platform-specific)
int get_time_ms();

// Check if input is waiting (for UCI communication)
int input_waiting();

// Read input from stdin
void read_input();

// Communicate (bridge between search and GUI input)
void communicate();

// Random number generation for Zobrist hashing
unsigned int get_random_U32_number();
U64 get_random_U64_number();
U64 generate_magic_number();

#endif // UTILS_HPP
