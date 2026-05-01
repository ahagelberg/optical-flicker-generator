#include "DeviceInfo.h"

static const char DEVICE_INFO_FIRMWARE_VERSION[] = "1.0";
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

const char* DeviceInfo::deviceType() {
    return DEVICE_INFO_TYPE;
}

const char* DeviceInfo::firmwareVersion() {
    return DEVICE_INFO_FIRMWARE_VERSION;
}
