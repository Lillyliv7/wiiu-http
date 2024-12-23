#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <stdbool.h>

#include <unordered_map>
#include <string>

#include "load_file.hpp"

#include "timestamp.hpp"

bool lastResWasCached = false;

uint64_t currentCacheSize = 0; // in bytes
uint64_t curr_cache_size = 0; // in elements
unsigned long long last_file_size = 0;
std::unordered_map<std::string, cachedFile_t> fileCache;

fileResponseStruct_t access_cache_entry(std::string filename) {
    fileResponseStruct_t res;
    res.read_fail = false;
    res.got_cached = true;
    lastResWasCached = true;
    res.cache_age = fileCache[filename].cached_at;
    fileCache[filename].accessed_at = curr_time;
    res.data_size = fileCache[filename].data_size;
    res.data = fileCache[filename].data;
    return res;
}

void erase_cache_entry(std::string filename) {
    fileCache[filename].useable = false;
    currentCacheSize -= fileCache[filename].data_size;
    free(fileCache[filename].data);
    fileCache.erase(filename);
}

// to be called periodically, removes all expired cache entries
void cull_cache() {
    curr_cache_size = fileCache.size();
    for (auto& pair : fileCache) {
        if (curr_time - pair.second.cached_at > MAX_CACHE_AGE_SECONDS) {
            // entry is too old, delete it

            erase_cache_entry(pair.first);
        }
    }
}

// false on fail
bool add_cache_entry(std::string filepath, uint64_t data_size, uint8_t* data) {
    if (currentCacheSize+data_size>MAX_CACHE_SIZE_BYTES) {
        // cache is full!
        return false;
    }
    auto element = fileCache.find(filepath);
    if (!(element == fileCache.end())) {
        // element already exists in cache

        // the only way this should be possible to trigger is
        // if someone accesses a file in the middle of it
        // being removed from cache, which should be quite rare.
        return false;
    }

    currentCacheSize += data_size;

    cachedFile_t cur_file;
    cur_file.accessed_at = curr_time;
    cur_file.cached_at = curr_time;
    cur_file.data = data;
    cur_file.data_size = data_size;
    cur_file.filepath = (char*)filepath.c_str();
    cur_file.useable = true;
    fileCache[filepath] = cur_file;

    return true;
}

// we have this function to read files instead of just using fopen() and fread() for caching pages
// do not free() res.data unless res.got_cached == false!!
fileResponseStruct_t read_file(const char* filename) {
    fileResponseStruct_t res;

    res.read_fail = false;
    res.got_cached = false;
    res.cache_age = 0;

    // check if file exists in cache

    std::string filenameSTDString = filename;
    // "auto" here is actually std::unordered_map<std::string, cachedFile_t>::iterator
    auto element = fileCache.find(filenameSTDString);
    // if element exists in cache (".end()" returns a value 1 outside the cache, which cant exist)
    if (!(element == fileCache.end())) {
        // check cache entry age
        if (!(curr_time - fileCache[filename].cached_at > MAX_CACHE_AGE_SECONDS) && fileCache[filename].useable) {
            // cache entry isnt too old, use it

            return access_cache_entry(filenameSTDString);
        } else {
            // cache entry is too old, delete it

            erase_cache_entry(filenameSTDString);
        }

    }
    // file in cache is too old, doesnt exist, or is being deleted currently; add file to cache

    // allocate enough memory for the filepath
    char* filenameBuf = (char*)malloc(strlen(filename)+strlen(WWW_DIR)+1);
    // append the filepath to the www root directory
    strcpy(filenameBuf, WWW_DIR);
    strcat(filenameBuf, filename);

    FILE *fptr = fopen(filenameBuf, "r");
    // failed to read the file for some reason (probably because it doesnt exist)
    if (fptr == NULL) {
        free(filenameBuf);

        res.read_fail = true;
        res.got_cached = false;
        res.data_size = 0;
        res.data = NULL;

        return res;
    }

    // get file size and put in res.data_size
    fseek(fptr, 0, SEEK_END);
    res.data_size = ftell(fptr);
    last_file_size = res.data_size;
    fseek(fptr, 0, SEEK_SET);

    res.data = (uint8_t*)malloc(res.data_size+1);

    fread(res.data, res.data_size, 1, fptr);
    fclose(fptr);
    free(filenameBuf);

    // add file to cache
    if (!add_cache_entry((char*)filename, res.data_size, res.data)) {
        res.got_cached = false;
        lastResWasCached = false;
    } else {
        res.got_cached = true;
        lastResWasCached = true;
    }

    return res;
}
