#pragma once

#include <stdbool.h>
#include <stdint.h>

// todo: implement cache

// where all files to be served are located
// (root of sd card should always be "fs:/vol/external01")
#define WWW_DIR "fs:/vol/external01/www"

#define MAX_CACHE_SIZE_BYTES 100000

struct fileResponseStruct {
    bool read_fail; // true if failed
    uint64_t cache_age; // in seconds, 0 if not cached
    uint64_t data_size; // in bytes
    uint8_t* data;
}; typedef struct fileResponseStruct fileResponseStruct_t;

// we have this function to read files instead of just using fopen() and fread() for caching pages
// make sure to free() res.data!!
fileResponseStruct_t read_file(const char* filename);
