#pragma once

#include <stddef.h>
#include <stdbool.h>

struct buf_struct {
    size_t tvBufferSize;
    size_t padBufferSize;
    void* tvBuf;
    void* padBuf;
    bool bufErr;
}; typedef struct buf_struct buf_struct_t;

buf_struct_t init_buffers ( void );
