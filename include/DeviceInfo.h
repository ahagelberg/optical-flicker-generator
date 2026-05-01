#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <Arduino.h>

/* 32 hex chars + terminator for SAMD21 128-bit unique ID as text */
#define DEVICE_INFO_ID_HEX_BUFFER_LEN 33

class DeviceInfo {
public:
    static void writeDeviceIdHex(char* buf, size_t len);
    static const char* deviceType();
    static const char* firmwareVersion();
};

#endif
