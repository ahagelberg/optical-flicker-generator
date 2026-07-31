#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <Arduino.h>

/* Status/debug lines on Serial. Always prefixed so API clients can filter them out.
 * Format: "# <message>" — never starts with OK or ERROR. */
void debugLog(const char* msg);
void debugLogf(const char* fmt, ...);

#endif
