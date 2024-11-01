#include "local_ip.hpp"

#include <nn/ac.h>
#include <stdio.h>
#include <stdint.h>

// get local ip of the wii u with the autoconnect api of nintendo network
// returns a string formatted like "192.168.1.87"

char local_ip_string[20] = { 0 };

const char* get_local_ip() {
    unsigned int nn_startupid;
    ACInitialize();
    ACGetStartupId(&nn_startupid);
    ACConnectWithConfigId(nn_startupid);
    uint32_t local_ip;
    ACGetAssignedAddress(&local_ip);
    ACFinalize();
    sprintf(local_ip_string, "%u.%u.%u.%u", 
        (local_ip >> 24) & 0xFF,
        (local_ip >> 16) & 0xFF,
        (local_ip >>  8) & 0xFF,
        (local_ip >>  0) & 0xFF
    );

    return local_ip_string;
}
