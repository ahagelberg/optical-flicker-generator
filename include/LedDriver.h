#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <Arduino.h>

class LedDriver {
public:
    LedDriver();
    void begin();
    void setOff();
    void setConstant(uint8_t intensityPercent);
    void setFlicker(uint32_t freqHz, uint8_t dutyPercent, uint8_t intensityPercent);
    void setSinus(uint32_t freqHz, uint8_t intensityPercent);
    void setCarrierHz(uint32_t hz);
private:
    static void sinusTimerIsr();
    void setupPin();
    void setupTcc1CarrierPwm();
    void configureTc4AtRate(uint32_t matchHz, bool envelopeWithDuty, uint8_t dutyPercent);
    void setTcc1Compare(uint32_t cc);
    void setTcc1Enabled(bool on);
    static void flickerTimerIsr();
    static LedDriver* instance_;
    friend void TC4_Handler(void);
    bool useCarrier_;
    uint32_t carrierHz_;
    uint32_t carrierPer_;
    uint32_t flickerIntensityPercent_;
    bool sinusMode_;
    volatile uint8_t sinusIndex_;
};

#endif
