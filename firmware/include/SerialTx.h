#ifndef SERIAL_TX_H
#define SERIAL_TX_H

#include <Arduino.h>

/* Non-blocking USB-CDC transmit path.
 * Application code queues bytes here; serialTxPoll() pushes at most one USB
 * packet when the IN endpoint is free. Never waits for a host. */

void serialTxWrite(const char* s);
void serialTxWrite(const uint8_t* data, size_t len);
void serialTxPrintln(const char* s);

/* Drain queued bytes if the CDC IN bank is idle. Call from loop(). */
void serialTxPoll();

bool serialTxEmpty();

#endif
