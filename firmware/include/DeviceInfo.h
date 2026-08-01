#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <Arduino.h>
#include "Config.h"

/* 32 hex chars + terminator for SAMD21 128-bit unique ID as text */
#define DEVICE_INFO_ID_HEX_BUFFER_LEN 33
#define DEVICE_INFO_MDNS_HOSTNAME_BUFFER_LEN MDNS_HOSTNAME_BUFFER_LEN
#define DEVICE_INFO_MAC_LEN 6
/* "AA:BB:CC:DD:EE:FF" + NUL */
#define DEVICE_INFO_MAC_STRING_BUFFER_LEN 18

class DeviceInfo {
public:
    static void writeDeviceIdHex(char* buf, size_t len);
    static void writeMdnsHostname(char* buf, size_t len);
    /* Locally administered unicast MAC derived from the SAMD21 unique ID (W5500 has none). */
    static void writeMacAddress(uint8_t* mac, size_t len);
    static void writeMacAddressString(char* buf, size_t len);
    static const char* deviceType();
    static const char* firmwareVersion();
};

#endif
