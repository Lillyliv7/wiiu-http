#include <stdint.h>
#include <sys/time.h>

#include "timestamp.hpp"

// the purpose of this file is to have a global timestamp variable that gets updated in
// the main loop to prevent getting the timestamp over and over within the same second
// to save a little processing power

uint64_t curr_time = 0;

void get_curr_time(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    curr_time = tv.tv_sec;
}
