// http server for the wii u written in c

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

#include <thread>
#include <vector>
#include <map>
#include <string>

#include "local_ip.hpp"
#include "split_string.hpp"
#include "os_screen_help.hpp"
#include "load_file.hpp"

#define PORT 80
#define MAX_POLL_CLIENTS 5
#define SOCKET_NONBLOCK false
#define SERVER_IDENTIFIER "Server: LillyHTTP/0.1 (Wii U/CafeOS)\n"

// struct clientStruct {
//     int client_fd;
//     struct sockaddr_in client_address;
//     socklen_t client_addrlen;
// }; typedef struct clientStruct clientStruct_t;

// std::vector<clientStruct_t> clients;


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
    } else {
        return (char*)0;
    }

    strcat(httpHeader, SERVER_IDENTIFIER);
    // set Accept-Ranges to "bytes" if we are to ever enable partial downloading of files
    strcat(httpHeader, "Accept-Ranges: none\n"); 
    strcat(httpHeader, "Connection: close\n");
    strcat(httpHeader, "Content-Type: ");
    strcat(httpHeader, content_type);

    strcpy(lastRes, httpHeader);

    return httpHeader;
}

void handle_client() {
    int client = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

    // fix these mallocs at some point in time to prevent buffer overflow, quite easy to do here 😔
    char* req = (char*)malloc(10000);
    char* res = (char*)malloc(10000);

    recv(client, req, 10000, 0);

    std::string reqStdString = req;
    std::string del = "\n";
    std::vector<std::string> inReq = split_string(reqStdString, del);

    char* firstSpace = strchr(inReq[0].c_str(), ' ');
    char* lastSpace = strrchr(inReq[0].c_str(), ' ');

    firstSpace[0] = (char)NULL;
    lastSpace[0] = (char)NULL;
    firstSpace++;

    fileResponseStruct_t fileRes = read_file(firstSpace);

    if (fileRes.read_fail) {
        char* head = http_response_head_builder(400,"text/html");

        sprintf(res, "%s\n\n%s\n\n", head, HTTP_Not_Found_Res);
        write(client, res, strlen(res));

        close(client);

        free(res);
        free(req);
        free(head);
        free(fileRes.data);
        connections++;
        return;
    }

    char* head = http_response_head_builder(200,"text/html");
    sprintf(res, "%s\n\n%s\n\n", head, fileRes.data);

    write(client, res, strlen(res));

    close(client);

    free(fileRes.data);
    free(res);
    free(req);
    free(head);
    connections++;
}

void handle_connection() {
    // struct pollfd fd[MAX_POLL_CLIENTS];
    // for (unsigned int i = 0; i < MAX_POLL_CLIENTS; i++) {
    //     fd[i].fd = server_fd;
    //     fd[i].events = POLLIN;
    // }

    while (shouldStopThread == false) {
        // poll connections
        struct pollfd fd;
        fd.fd = server_fd;
        fd.events = POLLIN;

        // if (poll(fd, MAX_POLL_CLIENTS, 500 /* 0.5s */) == -1) {
        if (poll(&fd, 1, 500 /* 0.5s */) == -1) {
            poll_err = true;
        }

        // loop through fd array and check for connection
        // for (unsigned int i = 0; i < MAX_POLL_CLIENTS; i++) {
            // if (fd[i].revents & POLLIN) {
            if (fd.revents & POLLIN) {
                // int client = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

                // // fix these mallocs at some point in time to prevent buffer overflow, quite easy to do here 😔
                // char* req = (char*)malloc(10000);
                // char* res = (char*)malloc(10000);

                // recv(client, req, 10000, 0);

                // char* head = http_response_head_builder(200,"text/html");
                
                // sprintf(res, "%s\n\n%s\n\n", head, req);

                // write(client, res, strlen(res));
                // close(client);

                // free(res);
                // free(req);
                // free(head);
                // connections++;
                std::thread tempThread(handle_client);
                tempThread.detach();
            }
        // }
        
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

    // unsigned long connections = 0;
    char* connectionsString = (char*)malloc(50);

    std::thread HandleConnectionThread(handle_connection);

    // FILE *fptr = fopen("fs:/vol/external01/test.txt", "r");

    // // Store the content of the file
    // char myString[100]; 

    // fgets(myString, 100, fptr);

    // fclose(fptr);

    while (WHBProcIsRunning()) { // main loop

        // clear screens
        OSScreenClearBufferEx(SCREEN_TV, 0); // 0 for clear to black
        OSScreenClearBufferEx(SCREEN_DRC, 0);

        if (flags_err) {
            OSScreenPutFontEx(SCREEN_TV, 0, 1, "Error setting socket to non-blocking mode");
        } else if (poll_err) {
            OSScreenPutFontEx(SCREEN_TV, 0, 1, "Poll() error");
        }

        OSScreenPutFontEx(SCREEN_TV, 0, 0, "LillyHTTP ver. 0.3");
        // OSScreenPutFontEx(SCREEN_TV, 0, 2, myString);

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

    // this line hangs for some reason 🙁

    // OSScreenShutdown();

    WHBLogPrint("Quitting safely");

    if (bufs.padBuf) free(bufs.padBuf);
    if (bufs.tvBuf) free(bufs.tvBuf);

    free(connectionsString);
    free((void*)local_ip_string);
    
    WHBLogCafeDeinit();
    WHBLogUdpDeinit();

    WHBProcShutdown();

    return 0;
}
