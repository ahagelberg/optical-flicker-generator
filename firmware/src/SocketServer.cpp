#include "SocketServer.h"
#include "CommandShell.h"
#include "Config.h"
#include "DebugLog.h"

SocketServer::SocketServer(CommandShell& shell)
    : shell_(shell), server_(SOCKET_PORT), clientActive_(false), lineLen_(0), lastReceivedMs_(0) {
    lineBuffer_[0] = '\0';
}

void SocketServer::begin() {
    server_.begin();
    debugLogf("socket listen port=%u", (unsigned)SOCKET_PORT);
}

void SocketServer::stopClient() {
    if (!clientActive_)
        return;
    client_.stop();
    clientActive_ = false;
    lineLen_ = 0;
    lineBuffer_[0] = '\0';
    debugLog("socket client disconnected");
}

void SocketServer::poll() {
    const unsigned long now = millis();
    if (!client_ || !client_.connected()) {
        if (clientActive_)
            stopClient();
        client_ = server_.accept();
        lineLen_ = 0;
        lineBuffer_[0] = '\0';
        lastReceivedMs_ = now;
        if (client_) {
            clientActive_ = true;
            debugLog("socket client connected");
        }
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
                shell_.executeAndReply(lineBuffer_, client_);
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
