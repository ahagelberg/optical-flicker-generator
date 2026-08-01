#include "DeviceInfo.h"
#include "Config.h"

static const char DEVICE_INFO_TYPE[] = "FLICKER_DEVICE";

#define SAMD21_SERIAL_WORD_0 *(volatile uint32_t*)(0x0080A00C)
#define SAMD21_SERIAL_WORD_1 *(volatile uint32_t*)(0x0080A040)
#define SAMD21_SERIAL_WORD_2 *(volatile uint32_t*)(0x0080A044)
#define SAMD21_SERIAL_WORD_3 *(volatile uint32_t*)(0x0080A048)

static void utox8(uint32_t val, char* out) {
    for (int i = 7; i >= 0; i--) {
        uint8_t d = val & 0xFu;
        out[i] = d > 9 ? (char)('A' + d - 10) : (char)('0' + d);
        val >>= 4;
    }
}

void DeviceInfo::writeDeviceIdHex(char* buf, size_t len) {
    if (buf == nullptr || len < DEVICE_INFO_ID_HEX_BUFFER_LEN) return;
    utox8(SAMD21_SERIAL_WORD_0, &buf[0]);
    utox8(SAMD21_SERIAL_WORD_1, &buf[8]);
    utox8(SAMD21_SERIAL_WORD_2, &buf[16]);
    utox8(SAMD21_SERIAL_WORD_3, &buf[24]);
    buf[32] = '\0';
}

void DeviceInfo::writeMdnsHostname(char* buf, size_t len) {
    if (buf == nullptr || len < DEVICE_INFO_MDNS_HOSTNAME_BUFFER_LEN) return;
    char idHex[DEVICE_INFO_ID_HEX_BUFFER_LEN];
    writeDeviceIdHex(idHex, sizeof(idHex));
    memcpy(buf, MDNS_HOSTNAME_PREFIX, MDNS_HOSTNAME_PREFIX_LEN);
    const char* suffix = &idHex[32 - MDNS_HOSTNAME_SUFFIX_LEN];
    for (uint8_t i = 0; i < MDNS_HOSTNAME_SUFFIX_LEN; i++) {
        char c = suffix[i];
        if (c >= 'A' && c <= 'F')
            c = (char)(c + ('a' - 'A'));
        buf[MDNS_HOSTNAME_PREFIX_LEN + i] = c;
    }
    buf[MDNS_HOSTNAME_PREFIX_LEN + MDNS_HOSTNAME_SUFFIX_LEN] = '\0';
}

void DeviceInfo::writeMacAddress(uint8_t* mac, size_t len) {
    if (mac == nullptr || len < DEVICE_INFO_MAC_LEN) return;
    /* Last 6 bytes of the 128-bit unique ID; mark locally administered unicast. */
    const uint32_t w2 = SAMD21_SERIAL_WORD_2;
    const uint32_t w3 = SAMD21_SERIAL_WORD_3;
    mac[0] = (uint8_t)(((w2 >> 16) & 0xFCu) | 0x02u);
    mac[1] = (uint8_t)(w2 >> 8);
    mac[2] = (uint8_t)(w2);
    mac[3] = (uint8_t)(w3 >> 24);
    mac[4] = (uint8_t)(w3 >> 16);
    mac[5] = (uint8_t)(w3 >> 8);
}

void DeviceInfo::writeMacAddressString(char* buf, size_t len) {
    if (buf == nullptr || len < DEVICE_INFO_MAC_STRING_BUFFER_LEN) {
        if (buf != nullptr && len > 0) buf[0] = '\0';
        return;
    }
    uint8_t mac[DEVICE_INFO_MAC_LEN];
    writeMacAddress(mac, sizeof(mac));
    snprintf(buf, len, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

const char* DeviceInfo::deviceType() {
    return DEVICE_INFO_TYPE;
}

const char* DeviceInfo::firmwareVersion() {
    return FIRMWARE_VERSION;
}
