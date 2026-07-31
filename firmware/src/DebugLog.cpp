#include "DebugLog.h"
#include "SerialTx.h"
#include <stdarg.h>

/* Distinct from ASCII shell OK/ERROR responses; easy to strip with ^# */
static const char DEBUG_LOG_PREFIX[] = "# ";
static const size_t DEBUG_LOG_MSG_MAX = 160;

void debugLog(const char* msg) {
    serialTxWrite(DEBUG_LOG_PREFIX);
    serialTxWrite(msg ? msg : "");
    serialTxWrite("\r\n");
}

void debugLogf(const char* fmt, ...) {
    char buf[DEBUG_LOG_MSG_MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    debugLog(buf);
}
