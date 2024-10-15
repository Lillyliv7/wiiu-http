// http server for the wii u written in c

// thank you https://github.com/MatthewL246/wiiu-hello-world
// for some tedious boilerplate

#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>

#include <coreinit/cache.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <vpad/input.h>
#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_console.h>
#include <whb/log_udp.h>
#include <whb/proc.h>
#include <arpa/inet.h>


#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <thread>


#define PORT 8080
#define SIZE 1024
#define MAX_CLIENTS 10
#define SOCKET_NONBLOCK false

size_t tvBufferSize;
size_t padBufferSize;
void* tvBuf;
void* padBuf;

bool shouldStopThread = false;

int server_fd;
unsigned long connections = 0;
struct sockaddr_in address;
int opt = 1;
socklen_t addrlen = sizeof(address);

const char* res200 = "HTTP/1.1 200 OK\n"
                    "Date: Mon, 28 May 2024 06:11:52 GMT\n"
                    "Server: LillyHTTP/2.4.54 (CafeOS)\n"
                    "Accept-Ranges: bytes\n"
                    "Connection: close\n"
                    "Content-Type: text/html\n";

void init_buffers ( void ) {
    tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    padBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

    WHBLogPrintf("Allocating 0x%X bytes for TV graphics and 0x%X bytes for Gamepad graphics", tvBufferSize, padBufferSize);

    tvBuf = memalign(0x100, tvBufferSize); // allocate tv and gamepad buffers aligned to 0x100 bytes
    padBuf = memalign(0x100, padBufferSize);

    if (!tvBuf || !padBuf) { // failed to allocate memory, likely because of no free memory left (turn off some aroma plugins or something!)
        WHBLogPrintf("Out of memory! Failed to allocate 0x%X bytes", tvBufferSize+padBufferSize);

        // free memory because CafeOS might not clean up for us 😔
        if (padBuf) free(padBuf);
        if (tvBuf) free(tvBuf);

        OSScreenShutdown();

        WHBLogPrint("Quitting with error");
        WHBLogCafeDeinit();
        WHBLogUdpDeinit();

        WHBProcShutdown();

        return;
    }

    WHBLogPrintf("Successfully allocated 0x%X bytes for graphics", tvBufferSize+padBufferSize);

    // buffers are set up, now bind them

    OSScreenSetBufferEx(SCREEN_TV, tvBuf);
    OSScreenSetBufferEx(SCREEN_DRC, padBuf);
    OSScreenEnableEx(SCREEN_TV, true);
    OSScreenEnableEx(SCREEN_DRC, true);
}

void handle_connection() {
    while (shouldStopThread == false) {
        // poll connections
        struct pollfd fd;
        fd.fd = server_fd;
        fd.events = POLLIN;

        if (poll(&fd, 1u, 500 /* 0.5s */) == -1) {
            OSScreenPutFontEx(SCREEN_TV, 0, 1, "error with poll");
        }

        // if we got a connection
        if (fd.revents & POLLIN) {
            int client = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

            // fix these mallocs at some point in time to prevent buffer overflow, quite easy to do here 😔
            char* req = (char*)malloc(10000);
            char* res = (char*)malloc(4000);

            recv(client, req, 10000, 0);
            
            sprintf(res, "%s\n\n%s\n\n", res200, req);

            write(client, res, strlen(res));
            close(client);

            free(res);
            free(req);
            connections++;
        }
        
    }
}

int main ( void ) {
    WHBProcInit(); // initialize Cafe OS stuff for us 😊

    WHBLogCafeInit(); // logs or something
    WHBLogUdpInit();
    WHBLogPrint("Starting HTTP server");

    OSScreenInit(); // start graphics

    init_buffers(); // init buffers for gamepad and tv

    // int server_fd;
    // struct sockaddr_in address;
    // int opt = 1;
    // socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int flags_err = 0;

    // set socket to non blocking
    if (SOCKET_NONBLOCK) {
        int flags = fcntl(server_fd, F_GETFL, 0);  // Get the current flags of the socket
        if (flags == -1) {
            OSScreenPutFontEx(SCREEN_TV, 0, 1, "Error getting socket flags");
            flags_err = 1;
        } else {
            // Set the socket to non-blocking mode
            if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                OSScreenPutFontEx(SCREEN_TV, 0, 1, "Error setting socket to non-blocking mode");
                flags_err = 1;
            }
        }
    }

    // unsigned long connections = 0;
    char* connectionsString = (char*)malloc(50);

    std::thread t(handle_connection);

    while (WHBProcIsRunning()) { // main loop

        // clear screens
        OSScreenClearBufferEx(SCREEN_TV, 0); // 0 for clear to black
        OSScreenClearBufferEx(SCREEN_DRC, 0);

        if (flags_err) {
            OSScreenPutFontEx(SCREEN_TV, 0, 1, "Error setting socket to non-blocking mode");
        }


        OSScreenPutFontEx(SCREEN_TV, 0, 0, "LillyHTTP ver. 0.3");

        sprintf(connectionsString, "Total connections: %ld", connections);
        OSScreenPutFontEx(SCREEN_DRC, 0, 0, connectionsString);

        DCFlushRange(tvBuf, tvBufferSize);
        DCFlushRange(padBuf, padBufferSize);

        // flip buffers to show graphics
        OSScreenFlipBuffersEx(SCREEN_TV);
        OSScreenFlipBuffersEx(SCREEN_DRC);

        OSSleepTicks(OSMillisecondsToTicks(50));
    }
    shouldStopThread = true;
    t.join();

    WHBLogPrint("Quitting safely");

    if (padBuf) free(padBuf);
    if (tvBuf) free(tvBuf);

    free(connectionsString);
    
    WHBLogCafeDeinit();
    WHBLogUdpDeinit();

    WHBProcShutdown();

    return 0;
}