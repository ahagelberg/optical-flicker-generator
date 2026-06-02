#ifndef RESET_BUTTON_H
#define RESET_BUTTON_H

#include <Arduino.h>

class ConfigStore;

class ResetButton {
public:
    explicit ResetButton(ConfigStore& config);
    void begin();
    void poll();
private:
    ConfigStore& config_;
    unsigned long pressedSinceMs_;
    bool wasPressed_;
};

#endif
