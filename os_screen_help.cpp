
#include <whb/log_cafe.h>
#include <whb/log_console.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <whb/proc.h>
#include <coreinit/screen.h>
#include <malloc.h>
#include <stdbool.h>

#include "os_screen_help.hpp"

buf_struct_t init_buffers () {
    buf_struct_t bufs;
    bufs.bufErr = FALSE;

    bufs.tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    bufs.padBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

    WHBLogPrintf("Allocating 0x%X bytes for TV graphics and 0x%X bytes for Gamepad graphics", bufs.tvBufferSize, bufs.padBufferSize);

    bufs.tvBuf = memalign(0x100, bufs.tvBufferSize); // allocate tv and gamepad buffers aligned to 0x100 bytes
    bufs.padBuf = memalign(0x100, bufs.padBufferSize);

    if (!bufs.tvBuf || !bufs.padBuf) { 
        // failed to allocate memory, likely because of no free memory left 
        // (turn off some aroma plugins or something!)
        WHBLogPrintf("Out of memory! Failed to allocate 0x%X bytes", bufs.tvBufferSize+bufs.padBufferSize);

        if (bufs.padBuf) free(bufs.padBuf);
        if (bufs.tvBuf) free(bufs.tvBuf);

        OSScreenShutdown();

        WHBLogPrint("Quitting with error");
        WHBLogCafeDeinit();
        WHBLogUdpDeinit();

        WHBProcShutdown();
        bufs.bufErr = TRUE;
        return bufs;
    }

    WHBLogPrintf("Successfully allocated 0x%X bytes for graphics", bufs.tvBufferSize+bufs.padBufferSize);

    // buffers are set up, now bind them

    OSScreenSetBufferEx(SCREEN_TV, bufs.tvBuf);
    OSScreenSetBufferEx(SCREEN_DRC, bufs.padBuf);
    OSScreenEnableEx(SCREEN_TV, true);
    OSScreenEnableEx(SCREEN_DRC, true);

    return bufs;
}
