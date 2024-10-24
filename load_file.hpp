#pragma once

#include <stdbool.h>
#include <stdint.h>

// todo: implement auto culling of old cache entries instead of waiting for them to be accessed


// where all files to be served are located
// (root of sd card should always be "fs:/vol/external01")
#define WWW_DIR "fs:/vol/external01/www"

// max size of the whole cache
#define MAX_CACHE_SIZE_BYTES 100000


// max age per file
// if a file in the cache is older than 60 seconds it only gets reloaded from disk
// when someone requests it to cut down on disk load
#define MAX_CACHE_AGE_SECONDS 60

extern bool lastResWasCached;

struct fileResponseStruct {
    bool read_fail; // true if failed
    bool got_cached; // true if the file was cached, if false, free res.data after using
    uint64_t cache_age; // in seconds, 0 if not cached
    uint64_t data_size; // in bytes
    uint8_t* data;
}; typedef struct fileResponseStruct fileResponseStruct_t;

struct cachedFile {
    char* filepath;
    bool useable; // set to false while the entry is being deleted from cache to
                  // prevent another thread from accessing data that has been freed
    uint64_t cached_at; // unix timestamp of when the data was cached
    uint64_t accessed_at; // last unix timestamp the file was accessed at
    uint64_t data_size;
    uint8_t* data;
}; typedef struct cachedFile cachedFile_t;

// self explanitory, returns false if failed to add to cache (not enough memory)
bool add_cache_entry(char* filepath, uint64_t data_size, uint8_t* data);

// we have this function to read files instead of just using fopen() and fread() for caching pages
// make sure to free() res.data!!
fileResponseStruct_t read_file(const char* filename);
