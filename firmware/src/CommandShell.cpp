#include "CommandShell.h"
#include "FrequencyCalibration.h"
#include "ConfigStore.h"
#include "DeviceInfo.h"
#include "Config.h"

/* Nominal Hz targets for the calibration wizard. Must not exceed CAL_MAX_POINTS. */
static const uint16_t kCalNominalHz[] = {1, 5, 10, 25, 50, 100, 200, 500};
static const uint8_t kCalStepCount = (uint8_t)(sizeof(kCalNominalHz) / sizeof(kCalNominalHz[0]));

static void toLower(char* s) {
    for (; *s; s++) {
        if (*s >= 'A' && *s <= 'Z') *s = (char)(*s + 32);
    }
}

CommandShell::CommandShell(FlickerController& flicker, FrequencyCalibration& cal, ConfigStore& store)
    : flicker_(flicker), cal_(cal), store_(store),
      wizardActive_(false), wizardStepIdx_(0),
      wizardSavedMode_(FlickerMode::Off), wizardSavedFreqHz_(DEFAULT_FREQ_HZ) {}

void CommandShell::executeLine(const char* line, char* buf, size_t len) {
    if (!line || !buf || len == 0) return;
    if (wizardActive_) {
        wizardHandleMeasurement(line, buf, len);
        return;
    }
    char cmd[PROTOCOL_PARSE_CMD_MAX];
    char args[PROTOCOL_PARSE_ARGS_MAX];
    args[0] = '\0';
    char scanFmt[24];
    snprintf(scanFmt, sizeof(scanFmt), "%%%us %%%u[^\n]",
             (unsigned)(PROTOCOL_PARSE_CMD_MAX - 1), (unsigned)(PROTOCOL_PARSE_ARGS_MAX - 1));
    int n = sscanf(line, scanFmt, cmd, args);
    if (n < 1) {
        snprintf(buf, len, "ERROR empty command");
        return;
    }
    toLower(cmd);
    const char* a = n >= 2 ? args : "";
    if (strcmp(cmd, "identify") == 0)                            return cmdIdentify(buf, len);
    if (strcmp(cmd, "mode") == 0)                                return cmdMode(a, buf, len);
    if (strcmp(cmd, "frequency") == 0 || strcmp(cmd, "freq") == 0) return cmdFrequency(a, buf, len);
    if (strcmp(cmd, "dutycycle") == 0 || strcmp(cmd, "duty") == 0) return cmdDutyCycle(a, buf, len);
    if (strcmp(cmd, "intensity") == 0 || strcmp(cmd, "int") == 0)  return cmdIntensity(a, buf, len);
    if (strcmp(cmd, "carrier") == 0)                               return cmdCarrier(a, buf, len);
    if (strcmp(cmd, "screensaver") == 0)                           return cmdScreensaver(a, buf, len);
    /* Check "calibration" before "calibrate" to avoid prefix collision. */
    if (strcmp(cmd, "calibration") == 0)                         return cmdCalibration(buf, len);
    if (strcmp(cmd, "calibrate") == 0)                           return cmdCalibrate(buf, len);
    snprintf(buf, len, "ERROR unknown command");
}

void CommandShell::cmdIdentify(char* buf, size_t len) {
    char id[DEVICE_INFO_ID_HEX_BUFFER_LEN];
    DeviceInfo::writeDeviceIdHex(id, sizeof(id));
    snprintf(buf, len, "OK %s %s %s",
             DeviceInfo::deviceType(), DeviceInfo::firmwareVersion(), id);
}

void CommandShell::cmdMode(const char* args, char* buf, size_t len) {
    if (args[0] == '\0') {
        snprintf(buf, len, "OK %s", flicker_.getModeString());
        return;
    }
    char mode[16];
    if (sscanf(args, "%15s", mode) != 1) {
        snprintf(buf, len, "ERROR invalid argument");
        return;
    }
    toLower(mode);
    if (strcmp(mode, "off") == 0) {
        flicker_.setOff();
    } else if (strcmp(mode, "constant") == 0) {
        flicker_.setConstant(flicker_.getIntensityPercent());
    } else if (strcmp(mode, "flicker") == 0) {
        flicker_.setFlicker(flicker_.getFrequencyHz(), flicker_.getDutyPercent(), flicker_.getIntensityPercent());
    } else if (strcmp(mode, "sinus") == 0) {
        flicker_.setSinus(flicker_.getFrequencyHz(), flicker_.getIntensityPercent());
    } else {
        snprintf(buf, len, "ERROR unknown mode (off constant flicker sinus)");
        return;
    }
    snprintf(buf, len, "OK");
}

void CommandShell::cmdFrequency(const char* args, char* buf, size_t len) {
    if (args[0] == '\0') {
        snprintf(buf, len, "OK %lu", (unsigned long)flicker_.getFrequencyHz());
        return;
    }
    unsigned int val;
    if (sscanf(args, "%u", &val) != 1 || !flicker_.trySetFrequency((uint32_t)val)) {
        snprintf(buf, len, "ERROR out of range (%u-%u Hz)", (unsigned)MIN_FREQ_HZ, (unsigned)MAX_FREQ_HZ);
        return;
    }
    snprintf(buf, len, "OK");
}

void CommandShell::cmdDutyCycle(const char* args, char* buf, size_t len) {
    if (args[0] == '\0') {
        snprintf(buf, len, "OK %u", flicker_.getDutyPercent());
        return;
    }
    unsigned int val;
    if (sscanf(args, "%u", &val) != 1 || !flicker_.trySetDutyCycle((uint8_t)val)) {
        snprintf(buf, len, "ERROR out of range (%u-%u %%)", (unsigned)MIN_DUTY_PERCENT, (unsigned)MAX_DUTY_PERCENT);
        return;
    }
    snprintf(buf, len, "OK");
}

void CommandShell::cmdIntensity(const char* args, char* buf, size_t len) {
    if (args[0] == '\0') {
        snprintf(buf, len, "OK %u", flicker_.getIntensityPercent());
        return;
    }
    unsigned int val;
    if (sscanf(args, "%u", &val) != 1 || !flicker_.trySetIntensity((uint8_t)val)) {
        snprintf(buf, len, "ERROR out of range (%u-%u %%)", (unsigned)MIN_INTENSITY_PERCENT, (unsigned)MAX_INTENSITY_PERCENT);
        return;
    }
    snprintf(buf, len, "OK");
}

void CommandShell::cmdCarrier(const char* args, char* buf, size_t len) {
    if (args[0] == '\0') {
        snprintf(buf, len, "OK %lu", (unsigned long)store_.getCarrierHz());
        return;
    }
    unsigned int val;
    if (sscanf(args, "%u", &val) != 1
            || val < MIN_CARRIER_HZ || val > MAX_CARRIER_HZ) {
        snprintf(buf, len, "ERROR out of range (%u-%u Hz)", (unsigned)MIN_CARRIER_HZ, (unsigned)MAX_CARRIER_HZ);
        return;
    }
    store_.setCarrierHz((uint32_t)val);
    flicker_.setCarrierHz(store_.getCarrierHz());
    store_.save();
    snprintf(buf, len, "OK");
}

void CommandShell::cmdScreensaver(const char* args, char* buf, size_t len) {
    if (args[0] == '\0') {
        snprintf(buf, len, "OK %u", (unsigned)store_.getScreensaverTimeoutS());
        return;
    }
    unsigned int val;
    if (sscanf(args, "%u", &val) != 1
            || val < MIN_DISPLAY_SCREENSAVER_S || val > MAX_DISPLAY_SCREENSAVER_S) {
        snprintf(buf, len, "ERROR out of range (%u-%u s)",
                 (unsigned)MIN_DISPLAY_SCREENSAVER_S, (unsigned)MAX_DISPLAY_SCREENSAVER_S);
        return;
    }
    store_.setScreensaverTimeoutS((uint16_t)val);
    store_.save();
    snprintf(buf, len, "OK");
}

void CommandShell::cmdCalibration(char* buf, size_t len) {
    size_t prefix = (size_t)snprintf(buf, len, "OK CAL ");
    if (prefix < len)
        cal_.writeTableString(buf + prefix, len - prefix);
}

void CommandShell::cmdCalibrate(char* buf, size_t len) {
    if (wizardActive_) {
        snprintf(buf, len, "ERROR wizard already active");
        return;
    }
    wizardSavedMode_ = flicker_.getMode();
    wizardSavedFreqHz_ = flicker_.getFrequencyHz();
    wizardStepIdx_ = 0;
    wizardActive_ = true;
    cal_.clear();
    /* Bypass calibration during wizard so we drive raw Hz and measure real error. */
    flicker_.setCalibrationBypass(true);
    /* Force flicker mode for stable envelope; duty/intensity remain as set by user. */
    flicker_.setFlicker(kCalNominalHz[0], flicker_.getDutyPercent(), flicker_.getIntensityPercent());
    wizardStartStep(buf, len);
}

void CommandShell::wizardHandleMeasurement(const char* line, char* buf, size_t len) {
    unsigned int measHz;
    if (sscanf(line, "%u", &measHz) != 1) {
        snprintf(buf, len, "ERROR expected measured Hz as integer");
        wizardStartStep(buf, len);  /* re-prompt current step */
        return;
    }
    cal_.addPoint(kCalNominalHz[wizardStepIdx_], (uint32_t)measHz);
    wizardStepIdx_++;
    if (wizardStepIdx_ >= kCalStepCount) {
        wizardFinish(buf, len);
    } else {
        flicker_.setFlicker(kCalNominalHz[wizardStepIdx_], flicker_.getDutyPercent(), flicker_.getIntensityPercent());
        wizardStartStep(buf, len);
    }
}

void CommandShell::wizardStartStep(char* buf, size_t len) {
    snprintf(buf, len, "OK WIZARD %u/%u %u",
             (unsigned)(wizardStepIdx_ + 1), (unsigned)kCalStepCount,
             (unsigned)kCalNominalHz[wizardStepIdx_]);
}

void CommandShell::wizardFinish(char* buf, size_t len) {
    wizardActive_ = false;
    flicker_.setCalibrationBypass(false);
    cal_.save(store_);
    /* Restore mode and user's desired frequency (now goes through calibration). */
    switch (wizardSavedMode_) {
        case FlickerMode::Off:
            flicker_.setOff();
            break;
        case FlickerMode::Constant:
            flicker_.setConstant(flicker_.getIntensityPercent());
            break;
        case FlickerMode::Flicker:
            flicker_.setFlicker(wizardSavedFreqHz_, flicker_.getDutyPercent(), flicker_.getIntensityPercent());
            break;
        case FlickerMode::Sinus:
            flicker_.setSinus(wizardSavedFreqHz_, flicker_.getIntensityPercent());
            break;
    }
    snprintf(buf, len, "OK WIZARD DONE");
}
