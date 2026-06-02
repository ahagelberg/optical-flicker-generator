#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <Arduino.h>
#include <Ethernet.h>
#include "Config.h"

class CommandShell;
class ConfigStore;

/* Thin I/O adapter: accepts one Ethernet client at a time on SOCKET_PORT,
   assembles lines, forwards to CommandShell (same commands as Serial). */
class SocketServer {
public:
    SocketServer(CommandShell& shell, ConfigStore& config);
    SocketServer(const SocketServer&) = delete;
    SocketServer& operator=(const SocketServer&) = delete;

    void begin();
    void poll();

private:
    CommandShell& shell_;
    ConfigStore& config_;
    EthernetServer server_;
    EthernetClient client_;
    char lineBuffer_[PROTOCOL_CMD_LINE_BUFFER_MAX];
    size_t lineLen_;
    unsigned long lastReceivedMs_;
};

#endif
