#ifndef SERIAL_COMMAND_H
#define SERIAL_COMMAND_H

#include <Arduino.h>
#include "Config.h"

class CommandShell;

/* Thin I/O adapter: reads bytes from Serial, assembles lines, forwards to CommandShell. */
class SerialCommand {
public:
    explicit SerialCommand(CommandShell& shell);
    SerialCommand(const SerialCommand&) = delete;
    SerialCommand& operator=(const SerialCommand&) = delete;

    void poll();

private:
    void processLine(const char* line);

    CommandShell& shell_;
    char lineBuffer_[PROTOCOL_CMD_LINE_BUFFER_MAX];
    size_t lineLen_;
    unsigned long lastReceivedMs_;
};

#endif
