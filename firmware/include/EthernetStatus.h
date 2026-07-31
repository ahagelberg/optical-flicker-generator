#ifndef ETHERNET_STATUS_H
#define ETHERNET_STATUS_H

#include <Arduino.h>
#include <IPAddress.h>

/* Call once per main-loop iteration; refreshes cached link/IP/hardware state. */
void ethernetPoll();

/* Cached: W5x00 present, PHY link up, and non-zero IPv4 assigned. */
bool ethernetIsUp();

/* Cached: SPI chip responds (shield present). */
bool ethernetHardwarePresent();

/* Cached local IPv4 (0.0.0.0 when down / unknown). */
IPAddress ethernetLocalIp();

/* Stable labels: "none"|"W5100"|"W5200"|"W5500" */
const char* ethernetHardwareLabel();

/* Stable labels: "on"|"off"|"unknown" */
const char* ethernetLinkLabel();

#endif
