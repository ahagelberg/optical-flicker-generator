#ifndef NETWORK_FORMAT_H
#define NETWORK_FORMAT_H

#include <Arduino.h>
#include <IPAddress.h>
#include "Config.h"

inline void formatIpv4(const IPAddress& ip, char* buf, size_t len) {
    if (buf == nullptr || len < IPV4_STRING_BUFFER_LEN) {
        if (buf != nullptr && len > 0) buf[0] = '\0';
        return;
    }
    snprintf(buf, len, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

#endif
