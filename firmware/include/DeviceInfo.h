#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <Arduino.h>
#include "Config.h"

/* 32 hex chars + terminator for SAMD21 128-bit unique ID as text */
#define DEVICE_INFO_ID_HEX_BUFFER_LEN 33
#define DEVICE_INFO_MDNS_HOSTNAME_BUFFER_LEN MDNS_HOSTNAME_BUFFER_LEN

class DeviceInfo {
public:
    static void writeDeviceIdHex(char* buf, size_t len);
    static void writeMdnsHostname(char* buf, size_t len);
    static const char* deviceType();
    static const char* firmwareVersion();
};

#endif
