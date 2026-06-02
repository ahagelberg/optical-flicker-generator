#ifndef COMMAND_SHELL_H
#define COMMAND_SHELL_H

#include <Arduino.h>
#include "FlickerController.h"  /* also brings in FlickerMode enum */

class FrequencyCalibration;
class ConfigStore;

/* Owns ASCII command parse/dispatch, OK/ERROR reply formatting, and calibration wizard
   session state. Serial and socket transports forward complete lines here. */
class CommandShell {
public:
    CommandShell(FlickerController& flicker, FrequencyCalibration& cal, ConfigStore& store);
    CommandShell(const CommandShell&) = delete;
    CommandShell& operator=(const CommandShell&) = delete;

    /* Parse and execute one complete line; writes null-terminated reply into buf. */
    void executeLine(const char* line, char* buf, size_t len);

private:
    void cmdIdentify(char* buf, size_t len);
    void cmdMode(const char* args, char* buf, size_t len);
    void cmdFrequency(const char* args, char* buf, size_t len);
    void cmdDutyCycle(const char* args, char* buf, size_t len);
    void cmdIntensity(const char* args, char* buf, size_t len);
    void cmdCarrier(const char* args, char* buf, size_t len);
    void cmdScreensaver(const char* args, char* buf, size_t len);
    void cmdCalibration(char* buf, size_t len);
    void cmdCalibrate(char* buf, size_t len);
    void wizardHandleMeasurement(const char* line, char* buf, size_t len);
    void wizardStartStep(char* buf, size_t len);
    void wizardFinish(char* buf, size_t len);

    FlickerController& flicker_;
    FrequencyCalibration& cal_;
    ConfigStore& store_;

    bool wizardActive_;
    uint8_t wizardStepIdx_;
    FlickerMode wizardSavedMode_;
    uint32_t wizardSavedFreqHz_;
};

#endif
