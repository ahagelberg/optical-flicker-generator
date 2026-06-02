#include "SerialCommand.h"
#include "CommandShell.h"
#include "Config.h"

SerialCommand::SerialCommand(CommandShell& shell)
    : shell_(shell), lineLen_(0), lastReceivedMs_(0) {
    lineBuffer_[0] = '\0';
}

void SerialCommand::processLine(const char* line) {
    if (!line || line[0] == '\0') return;
    char response[PROTOCOL_RESPONSE_MAX];
    shell_.executeLine(line, response, sizeof(response));
    Serial.println(response);
}

void SerialCommand::poll() {
    const unsigned long now = millis();
    while (Serial.available() > 0) {
        lastReceivedMs_ = now;
        if (lineLen_ >= PROTOCOL_CMD_LINE_BUFFER_MAX - 1) {
            Serial.read();
            continue;
        }
        int c = Serial.read();
        if (c < 0) break;
        if (c == '\n' || c == '\r') {
            lineBuffer_[lineLen_] = '\0';
            if (lineLen_ > 0) {
                processLine(lineBuffer_);
                lineLen_ = 0;
            }
            while (Serial.available() > 0) {
                int d = Serial.peek();
                if (d == '\n' || d == '\r') Serial.read();
                else break;
            }
            continue;
        }
        lineBuffer_[lineLen_++] = (char)c;
    }
    if (lineLen_ > 0 && (now - lastReceivedMs_) >= (unsigned long)SERIAL_LINE_TIMEOUT_MS) {
        lineLen_ = 0;
    }
}
