#include "SerialTx.h"
#include "USB/USBAPI.h"
#include "USB/SAMD21_USBDevice.h"

extern USBDevice_SAMD21G18x usbd;

/* Stock SAMD core: first PluggableUSB module gets endpoint 1 → CDC IN = 3. */
static const uint32_t SERIAL_CDC_TX_EP = 3;
static const size_t SERIAL_TX_PACKET_MAX = EPX_SIZE;
static const size_t SERIAL_TX_RING_SIZE = 1024;

static uint8_t ring_[SERIAL_TX_RING_SIZE];
static size_t head_ = 0;
static size_t tail_ = 0;
static size_t count_ = 0;

static bool cdcTxBankIdle() {
    if (!USBDevice.configured())
        return false;
    /* BK1RDY set means a packet is still awaiting host ACK — Serial.write would block. */
    return !usbd.epBank1IsReady(SERIAL_CDC_TX_EP);
}

void serialTxWrite(const uint8_t* data, size_t len) {
    if (!data || len == 0)
        return;
    for (size_t i = 0; i < len; i++) {
        if (count_ >= SERIAL_TX_RING_SIZE) {
            tail_ = (tail_ + 1) % SERIAL_TX_RING_SIZE;
            count_--;
        }
        ring_[head_] = data[i];
        head_ = (head_ + 1) % SERIAL_TX_RING_SIZE;
        count_++;
    }
}

void serialTxWrite(const char* s) {
    if (!s)
        return;
    serialTxWrite(reinterpret_cast<const uint8_t*>(s), strlen(s));
}

void serialTxPrintln(const char* s) {
    serialTxWrite(s);
    serialTxWrite(reinterpret_cast<const uint8_t*>("\r\n"), 2);
}

bool serialTxEmpty() {
    return count_ == 0;
}

void serialTxPoll() {
    if (count_ == 0 || !cdcTxBankIdle())
        return;

    uint8_t pkt[SERIAL_TX_PACKET_MAX];
    size_t n = count_;
    if (n > SERIAL_TX_PACKET_MAX)
        n = SERIAL_TX_PACKET_MAX;
    for (size_t i = 0; i < n; i++) {
        pkt[i] = ring_[tail_];
        tail_ = (tail_ + 1) % SERIAL_TX_RING_SIZE;
    }
    count_ -= n;
    /* Bank is idle and payload fits one packet — USBDevice::send will not wait. */
    Serial.write(pkt, n);
}
