#include "local_ip.hpp"

#include <nn/ac.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

// get local ip of the wii u with the autoconnect api of nintendo network
// returns a string formatted like "192.168.1.87"

const char* get_local_ip() {
    unsigned int nn_startupid;
    ACInitialize();
    ACGetStartupId(&nn_startupid);
    ACConnectWithConfigId(nn_startupid);
    uint32_t local_ip;
    ACGetAssignedAddress(&local_ip);
    ACFinalize();
    char* local_ip_string = (char*)malloc(20);
    sprintf(local_ip_string, "%u.%u.%u.%u", 
        (local_ip >> 24) & 0xFF,
        (local_ip >> 16) & 0xFF,
        (local_ip >>  8) & 0xFF,
        (local_ip >>  0) & 0xFF
    );

    return local_ip_string;
}
