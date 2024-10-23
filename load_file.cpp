#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "load_file.hpp"

// todo: implement cache

// we have this function to read files instead of just using fopen() and fread() for caching pages
// make sure to free() res.data!!
fileResponseStruct_t read_file(const char* filename) {
    fileResponseStruct_t res;

    res.read_fail = false;
    res.cache_age = 0;

    // allocate enough memory for the filepath
    char* filenameBuf = (char*)malloc(strlen(filename)+strlen(WWW_DIR)+1);
    // append the filepath to the www root directory
    strcpy(filenameBuf, WWW_DIR);
    strcat(filenameBuf, filename);

    FILE *fptr = fopen(filenameBuf, "rb");
    // failed to read the file for some reason (probably because it doesnt exist)
    if (fptr == NULL) {
        free(filenameBuf);

        res.read_fail = true;
        res.data_size = 0;
        res.data = NULL;

        return res;
    }

    // get file size and put in res.data_size
    fseek(fptr, 0, SEEK_END);
    res.data_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    res.data = (uint8_t*)malloc(res.data_size);

    fread(res.data, res.data_size, 1, fptr);

    fclose(fptr);

    free(filenameBuf);
    return res;
}
