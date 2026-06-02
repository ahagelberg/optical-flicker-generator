#include "SocketServer.h"
#include "CommandShell.h"
#include "ConfigStore.h"
#include "Config.h"

SocketServer::SocketServer(CommandShell& shell, ConfigStore& config)
    : shell_(shell), config_(config), server_(SOCKET_PORT), lineLen_(0), lastReceivedMs_(0) {
    lineBuffer_[0] = '\0';
}

void SocketServer::begin() {
    server_.begin();
}

void SocketServer::poll() {
    const unsigned long now = millis();
    if (!config_.getTelnetEnabled()) {
        if (client_ && client_.connected()) client_.stop();
        lineLen_ = 0;
        lineBuffer_[0] = '\0';
        return;
    }
    if (!client_ || !client_.connected()) {
        client_ = server_.accept();
        lineLen_ = 0;
        lineBuffer_[0] = '\0';
        lastReceivedMs_ = now;
        return;
    }
    while (client_.available() > 0) {
        lastReceivedMs_ = now;
        if (lineLen_ >= PROTOCOL_CMD_LINE_BUFFER_MAX - 1) {
            client_.read();
            continue;
        }
        int c = client_.read();
        if (c < 0) break;
        if (c == '\n' || c == '\r') {
            lineBuffer_[lineLen_] = '\0';
            if (lineLen_ > 0) {
                char response[PROTOCOL_RESPONSE_MAX];
                shell_.executeLine(lineBuffer_, response, sizeof(response));
                client_.println(response);
                lineLen_ = 0;
            }
            while (client_.available() > 0) {
                int d = client_.peek();
                if (d == '\n' || d == '\r') client_.read();
                else break;
            }
            continue;
        }
        lineBuffer_[lineLen_++] = (char)c;
    }
    if (lineLen_ > 0 && (now - lastReceivedMs_) >= (unsigned long)SERIAL_LINE_TIMEOUT_MS) {
        lineLen_ = 0;
        lineBuffer_[0] = '\0';
    }
}
