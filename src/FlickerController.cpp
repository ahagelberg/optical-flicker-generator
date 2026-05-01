#include "FlickerController.h"
#include "FrequencyCalibration.h"
#include "LedDriver.h"
#include "Config.h"

FlickerController::FlickerController(LedDriver& driver, FrequencyCalibration& cal)
    : driver_(driver), cal_(cal), mode_(FlickerMode::Off),
      freqHz_(DEFAULT_FREQ_HZ), dutyPercent_(DEFAULT_DUTY_PERCENT),
      intensityPercent_(DEFAULT_INTENSITY_PERCENT), calibrationBypass_(false) {}

void FlickerController::begin() {
    driver_.begin();
}

void FlickerController::setOff() {
    mode_ = FlickerMode::Off;
    driver_.setOff();
}

void FlickerController::setConstant(uint8_t intensityPercent) {
    mode_ = FlickerMode::Constant;
    intensityPercent_ = intensityPercent;
    clampAndApply();
}

void FlickerController::setFlicker(uint32_t freqHz, uint8_t dutyPercent, uint8_t intensityPercent) {
    mode_ = FlickerMode::Flicker;
    freqHz_ = freqHz;
    dutyPercent_ = dutyPercent;
    intensityPercent_ = intensityPercent;
    clampAndApply();
}

void FlickerController::setSinus(uint32_t freqHz, uint8_t intensityPercent) {
    mode_ = FlickerMode::Sinus;
    freqHz_ = freqHz;
    intensityPercent_ = intensityPercent;
    clampAndApply();
}

void FlickerController::setFrequency(uint32_t freqHz) {
    freqHz_ = freqHz;
    clampAndApply();
}

void FlickerController::setDutyCycle(uint8_t dutyPercent) {
    dutyPercent_ = dutyPercent;
    clampAndApply();
}

void FlickerController::setIntensity(uint8_t intensityPercent) {
    intensityPercent_ = intensityPercent;
    clampAndApply();
}

bool FlickerController::trySetFrequency(uint32_t freqHz) {
    if (freqHz < MIN_FREQ_HZ || freqHz > MAX_FREQ_HZ) return false;
    freqHz_ = freqHz;
    clampAndApply();
    return true;
}

bool FlickerController::trySetDutyCycle(uint8_t dutyPercent) {
    if (dutyPercent < MIN_DUTY_PERCENT || dutyPercent > MAX_DUTY_PERCENT) return false;
    dutyPercent_ = dutyPercent;
    clampAndApply();
    return true;
}

bool FlickerController::trySetIntensity(uint8_t intensityPercent) {
    if (intensityPercent > MAX_INTENSITY_PERCENT) return false;
    intensityPercent_ = intensityPercent;
    clampAndApply();
    return true;
}

void FlickerController::setCarrierHz(uint32_t hz) {
    driver_.setCarrierHz(hz);
    clampAndApply();
}

void FlickerController::setCalibrationBypass(bool bypass) {
    calibrationBypass_ = bypass;
}

void FlickerController::clampAndApply() {
    if (freqHz_ < MIN_FREQ_HZ) freqHz_ = MIN_FREQ_HZ;
    if (freqHz_ > MAX_FREQ_HZ) freqHz_ = MAX_FREQ_HZ;
    if (dutyPercent_ < MIN_DUTY_PERCENT) dutyPercent_ = MIN_DUTY_PERCENT;
    if (dutyPercent_ > MAX_DUTY_PERCENT) dutyPercent_ = MAX_DUTY_PERCENT;
    if (intensityPercent_ > MAX_INTENSITY_PERCENT) intensityPercent_ = MAX_INTENSITY_PERCENT;
    if (mode_ == FlickerMode::Flicker) {
        uint32_t driverHz = calibrationBypass_ ? freqHz_ : cal_.commandHzForDesired(freqHz_);
        driver_.setFlicker(driverHz, dutyPercent_, intensityPercent_);
    } else if (mode_ == FlickerMode::Constant) {
        driver_.setConstant(intensityPercent_);
    } else if (mode_ == FlickerMode::Sinus) {
        uint32_t driverHz = calibrationBypass_ ? freqHz_ : cal_.commandHzForDesired(freqHz_);
        driver_.setSinus(driverHz, intensityPercent_);
    }
}

const char* FlickerController::getModeString() const {
    switch (mode_) {
        case FlickerMode::Off:      return "off";
        case FlickerMode::Constant: return "constant";
        case FlickerMode::Flicker:  return "flicker";
        case FlickerMode::Sinus:    return "sinus";
        default:                    return "unknown";
    }
}
