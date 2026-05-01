#include "ResetButton.h"
#include "ConfigStore.h"
#include "Config.h"
#include <sam.h>

ResetButton::ResetButton(ConfigStore& config)
    : config_(config), pressedSinceMs_(0), wasPressed_(false) {}

void ResetButton::begin() {
    pinMode(PIN_RESET_BUTTON, INPUT_PULLUP);
}

void ResetButton::poll() {
    bool pressed = (digitalRead(PIN_RESET_BUTTON) == LOW);
    unsigned long now = millis();
    if (pressed) {
        if (!wasPressed_) {
            pressedSinceMs_ = now;
            wasPressed_ = true;
        } else if (now - pressedSinceMs_ >= BUTTON_HOLD_MS) {
            config_.resetToDefaults();
            config_.save();
            NVIC_SystemReset();
        }
    } else {
        wasPressed_ = false;
    }
}
