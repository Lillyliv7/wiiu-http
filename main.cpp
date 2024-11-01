// http server for the wii u written in c

// todo: improve displaying of info on tv and gamepad

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
#include <sysapp/launch.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>

#include <thread>
#include <vector>
#include <map>
#include <string>

#include "local_ip.hpp"
#include "split_string.hpp"
#include "os_screen_help.hpp"
#include "load_file.hpp"
#include "timestamp.hpp"
#include "http.hpp"
#include "string_startswith.hpp"

#define PORT 80
#define SOCKET_NONBLOCK false
#define SERVER_IDENTIFIER "Server: LillyHTTP/0.2 (Wii U/CafeOS)\n"

bool shouldStopThread = false;

int server_fd;
unsigned long connections = 0;
struct sockaddr_in address;
int opt = 1;
bool flags_err = false;
bool poll_err = false;
socklen_t addrlen = sizeof(address);

const char* HTTP_Not_Found_Res = "404 NOT FOUND!\n";

// last http response to print to console
char lastRes[1000] = { 0 };

// make sure to free() the string given to you by this!!
char* http_response_head_builder(int resCode, const char *content_type) {
    char *httpHeader = (char*)malloc(500); // 500 bytes should be plenty

    if (resCode == 200) {
        strcpy(httpHeader, "HTTP/1.1 200 OK\n");
    } else if (resCode == 404) {
        strcpy(httpHeader, "HTTP/1.1 404 Not Found\n");
    } else if (resCode == 405) {
        strcpy(httpHeader, "HTTP/1.1 405 Method Not Allowed\n");
    } else {
        return (char*)0;
    }

    strcat(httpHeader, SERVER_IDENTIFIER);
    // set Accept-Ranges to "bytes" if we are to ever enable partial downloading of files
    // strcat(httpHeader, "Content-Length: 100000\n"); 
    strcat(httpHeader, "Accept-Ranges: none\n"); 
    strcat(httpHeader, "Connection: close\n");
    strcat(httpHeader, "Content-Type: ");
    strcat(httpHeader, content_type);

    strcpy(lastRes, httpHeader);

    strcat(httpHeader, "\n\n");

    return httpHeader;
}


void handle_client() {
    int client = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

    char* req = (char*)malloc(10000);
    recv(client, req, 10000, 0);
    std::string reqStdstr = req;
    std::unordered_map<std::string, std::string> reqMap = LHTTP::get_HTTP_req_params(reqStdstr);

    auto httpMethodElement = reqMap.find("ReqMethod");
    if(httpMethodElement == reqMap.end()) {
        // empty map, user submitted an invalid request or get_HTTP_req_params broke
        // return error page

        char* head = http_response_head_builder(405,"text/html");

        write(client, head, strlen(head));
        write(client, HTTP_Not_Found_Res, strlen(HTTP_Not_Found_Res));

        close(client);

        free(req);
        free(head);
        connections++;
        lastResWasCached = false;
        return;
    }

    fileResponseStruct_t fileRes = read_file(reqMap["ReqPath"].c_str());

    if (fileRes.read_fail) {
        char* head = http_response_head_builder(404,"text/html");

        write(client, head, strlen(head));
        write(client, HTTP_Not_Found_Res, strlen(HTTP_Not_Found_Res));

        close(client);

        free(req);
        free(head);
        if (!fileRes.got_cached) free(fileRes.data);
        connections++;
        lastResWasCached = false;
        return;
    }

    char* head = http_response_head_builder(200,LHTTP::get_MIME_from_filename(reqMap["ReqPath"]).c_str());

    write(client, head, strlen(head));

    int64_t i = 0;
    int64_t size = fileRes.data_size;
    while (true) {
        if (size<500) {
            write(client, fileRes.data+i, size);
            break;
        }
        write(client, fileRes.data+i, 500);
        size-=500;
        i+=500;
    }

    close(client);

    // only free the data if it is not from the cache,
    // if the data is in the cache, it will get cleared
    // automatically when the entry expires
    if (!fileRes.got_cached) free(fileRes.data);

    free(req);
    free(head);
    connections++;
}


void handle_connection() {
    while (shouldStopThread == false) {
        // poll connections
        struct pollfd fd;
        fd.fd = server_fd;
        fd.events = POLLIN;

        if (poll(&fd, 1, 500 /* 0.5s */) == -1) {
            poll_err = true;
        }
            if (fd.revents & POLLIN) {
                std::thread tempThread(handle_client);
                tempThread.detach();
            }
    }
}

int initSocket() {
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return 1;

    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) return 1;


    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) return 1;

    if (listen(server_fd, 3) < 0) return 1;

    // set socket to non blocking if we have that enabled (broken...)
    if (SOCKET_NONBLOCK) {
        int flags = fcntl(server_fd, F_GETFL, 0);  // Get the current flags of the socket
        if (flags == -1) {
            flags_err = true;
            WHBLogPrint("Error getting socket flags");
        } else {
            // Set the socket to non-blocking mode
            if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                flags_err = true;
                WHBLogPrint("Error setting socket to non blocking mode");
            }
        }
    }

    return 0;
}

int main ( void ) {
    WHBProcInit(); // initialize Cafe OS stuff for us 😊

    WHBLogCafeInit(); // logs or something
    WHBLogUdpInit();
    WHBLogPrint("Starting HTTP server");

    const char* local_ip_string = get_local_ip();

    if (initSocket()) {
        WHBLogPrint("Error initializing socket");
        WHBLogCafeDeinit();
        WHBLogUdpDeinit();
        WHBProcShutdown();
        SYSLaunchMenu();
        return 1;
    }


    OSScreenInit(); // start graphics
    buf_struct_t bufs = init_buffers(); // init buffers for gamepad and tv
    if (bufs.bufErr) { // error allocating buffers
        return 1;
    }

    char* connectionsString = (char*)malloc(50);

    std::thread HandleConnectionThread(handle_connection);

    while (WHBProcIsRunning()) { // main loop
        get_curr_time(); // update global time variable
        cull_cache();

        // clear screens
        OSScreenClearBufferEx(SCREEN_TV, 0); // 0 for clear to black
        OSScreenClearBufferEx(SCREEN_DRC, 0);

        char cacheEntriesString[80] = {0};
        sprintf(cacheEntriesString, "cache entries: %llu", curr_cache_size);

        char cacheSizeString[80] = {0};
        sprintf(cacheSizeString, "current cache size (bytes): %llu", currentCacheSize);

        if (flags_err) {
            OSScreenPutFontEx(SCREEN_TV, 0, 1, "Error setting socket to non-blocking mode");
        } else if (poll_err) {
            OSScreenPutFontEx(SCREEN_TV, 0, 1, "Poll() error");
        }

        if (lastResWasCached) {
            OSScreenPutFontEx(SCREEN_TV, 0, 4, "last request was cached");
        } else {
            OSScreenPutFontEx(SCREEN_TV, 0, 4, "last request was not cached");
        }

        OSScreenPutFontEx(SCREEN_TV, 0, 0, "LillyHTTP ver. 0.3");
        OSScreenPutFontEx(SCREEN_TV, 0, 2, cacheEntriesString);
        OSScreenPutFontEx(SCREEN_TV, 0, 3, cacheSizeString);

        sprintf(connectionsString, "Total connections: %ld", connections);
        OSScreenPutFontEx(SCREEN_DRC, 0, 0, connectionsString);
        OSScreenPutFontEx(SCREEN_DRC, 45, 0, local_ip_string);
        OSScreenPutFontEx(SCREEN_DRC, 0, 1, lastRes);

        DCFlushRange(bufs.tvBuf, bufs.tvBufferSize);
        DCFlushRange(bufs.padBuf, bufs.padBufferSize);

        // flip buffers to show graphics
        OSScreenFlipBuffersEx(SCREEN_TV);
        OSScreenFlipBuffersEx(SCREEN_DRC);

        OSSleepTicks(OSMillisecondsToTicks(100));
    }

    shouldStopThread = true;
    HandleConnectionThread.join();

    close(server_fd);

    // this line hangs for some reason 🙁
    // OSScreenShutdown();

    WHBLogPrint("Quitting safely");

    if (bufs.padBuf) free(bufs.padBuf);
    if (bufs.tvBuf) free(bufs.tvBuf);

    free(connectionsString);
    
    WHBLogCafeDeinit();
    WHBLogUdpDeinit();

    WHBProcShutdown();

    return 0;
}
