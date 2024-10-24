#include <stdint.h>

#include "timestamp.hpp"

// the purpose of this file is to have a global timestamp variable that gets updated in
// the main loop to prevent getting the timestamp over and over within the same second
// to save a little processing power

uint64_t curr_time = 0;
