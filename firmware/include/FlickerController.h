#ifndef FLICKER_CONTROLLER_H
#define FLICKER_CONTROLLER_H

#include <Arduino.h>

class LedDriver;
class FrequencyCalibration;

enum class FlickerMode {
    Off,
    Constant,
    Flicker,
    Sinus
};

class FlickerController {
public:
    FlickerController(LedDriver& driver, FrequencyCalibration& cal);
    FlickerController(const FlickerController&) = delete;
    FlickerController& operator=(const FlickerController&) = delete;

    void begin();

    FlickerMode getMode() const { return mode_; }
    uint32_t getFrequencyHz() const { return freqHz_; }
    uint8_t getDutyPercent() const { return dutyPercent_; }
    uint8_t getIntensityPercent() const { return intensityPercent_; }

    void setOff();
    void setConstant(uint8_t intensityPercent);
    void setFlicker(uint32_t freqHz, uint8_t dutyPercent, uint8_t intensityPercent);
    void setSinus(uint32_t freqHz, uint8_t intensityPercent);
    void setFrequency(uint32_t freqHz);
    void setDutyCycle(uint8_t dutyPercent);
    void setIntensity(uint8_t intensityPercent);
    bool trySetFrequency(uint32_t freqHz);
    bool trySetDutyCycle(uint8_t dutyPercent);
    bool trySetIntensity(uint8_t intensityPercent);
    void setCarrierHz(uint32_t hz);

    /* Enable/disable calibration bypass for wizard measurement steps.
       When true, raw freqHz_ is sent to the driver without calibration. */
    void setCalibrationBypass(bool bypass);

    const char* getModeString() const;

private:
    void clampAndApply();

    LedDriver& driver_;
    FrequencyCalibration& cal_;
    FlickerMode mode_;
    uint32_t freqHz_;
    uint8_t dutyPercent_;
    uint8_t intensityPercent_;
    bool calibrationBypass_;
};

#endif
