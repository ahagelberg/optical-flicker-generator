#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

class FlickerController;
class ConfigStore;

class Display {
public:
    Display(FlickerController& flicker, ConfigStore& config);
    /* Call first in setup() before other slow work (e.g. Ethernet); powers OLED and shows power-on text. */
    void showBootSplash();
    /* Call after hardware init is complete; arms screensaver timing from normal UI. */
    void begin();
    void update();
private:
    void refreshFullRedraw(unsigned long now);
    FlickerController& flicker_;
    ConfigStore& config_;
    unsigned long lastUpdateMs_;
    unsigned long lastUserActivityMs_;
    bool screensaverOn_;
    bool prevButtonPressed_;
};

#endif
