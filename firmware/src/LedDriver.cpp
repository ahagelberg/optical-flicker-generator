#include "LedDriver.h"
#include "Config.h"
#include "wiring_private.h"
#include <sam.h>

static inline void syncTCC(Tcc* TCCx) {
    while (TCCx->SYNCBUSY.reg) {}
}

LedDriver* LedDriver::instance_ = nullptr;

static const uint32_t F_CPU_HZ = 48000000u;
/* SAMD21 TC prescaler enum values map to divisors {1,2,4,8,16,64,256,1024}, not 2^n for index ≥ 5 */
static const uint32_t TC4_PRESCALER_DIV[] = {1, 2, 4, 8, 16, 64, 256, 1024};
static const uint8_t TC4_PRESCALER_DIV_COUNT = 8;
static const uint8_t SINE_TABLE_SIZE = 64;
static const uint16_t sineTable[SINE_TABLE_SIZE] = {
    2048, 2210, 2370, 2527, 2680, 2829, 2972, 3109, 3239, 3362, 3477, 3584, 3682, 3771, 3850, 3920,
    3979, 4028, 4066, 4094, 4110, 4116, 4110, 4094, 4066, 4028, 3979, 3920, 3850, 3771, 3682, 3584,
    3477, 3362, 3239, 3109, 2972, 2829, 2680, 2527, 2370, 2210, 2048, 1886, 1726, 1569, 1416, 1267,
    1124, 987, 857, 734, 619, 512, 414, 325, 246, 176, 117, 68, 30, 2, 14, 30
};

LedDriver::LedDriver()
    : useCarrier_(false), carrierHz_(DEFAULT_CARRIER_HZ), carrierPer_(0),
      flickerIntensityPercent_(100), sinusMode_(false), sinusIndex_(0) {
    instance_ = this;
}

void LedDriver::begin() {
    setupPins();
    setOff();
}

void LedDriver::setupPins() {
    pinMode(PIN_LED_OUTPUT_A, OUTPUT);
    pinMode(PIN_LED_OUTPUT_B, OUTPUT);
    pinPeripheral(PIN_LED_OUTPUT_A, PIO_TIMER);
    pinPeripheral(PIN_LED_OUTPUT_B, PIO_TIMER);
}

void LedDriver::setTcc1Enabled(bool on) {
    if (on) {
        GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC0_TCC1));
        while (GCLK->STATUS.bit.SYNCBUSY) {}
        TCC1->CTRLA.bit.ENABLE = 1;
        syncTCC(TCC1);
    } else {
        TCC1->CTRLA.bit.ENABLE = 0;
        syncTCC(TCC1);
    }
}

/* Mirror compare to both WO outputs so D2 and D3 stay identical */
void LedDriver::setTcc1CompareBoth(uint32_t cc) {
    TCC1->CC[0].reg = cc;
    TCC1->CC[1].reg = cc;
    syncTCC(TCC1);
}

void LedDriver::setCarrierHz(uint32_t hz) {
    if (hz < MIN_CARRIER_HZ) hz = MIN_CARRIER_HZ;
    if (hz > MAX_CARRIER_HZ) hz = MAX_CARRIER_HZ;
    carrierHz_ = hz;
}

void LedDriver::setupTcc1CarrierPwm() {
    const uint32_t prescaler = 1;
    carrierPer_ = (F_CPU_HZ / 2) / (carrierHz_ * prescaler);
    if (carrierPer_ > 0xFFFFFFu) carrierPer_ = 0xFFFFFFu;
    TCC1->CTRLA.bit.ENABLE = 0;
    syncTCC(TCC1);
    GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TCC0_TCC1));
    while (GCLK->STATUS.bit.SYNCBUSY) {}
    TCC1->WAVE.reg = TCC_WAVE_WAVEGEN_DSBOTH;
    syncTCC(TCC1);
    TCC1->PER.reg = carrierPer_;
    syncTCC(TCC1);
    setTcc1CompareBoth(0);
    TCC1->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_ENABLE;
    syncTCC(TCC1);
}

/* MC0 / MC1 interrupts at flicker rate (duty split) or faster tick for sinus stepping */
void LedDriver::configureTc4AtRate(uint32_t matchHz, bool envelopeWithDuty, uint8_t dutyPercent) {
    PM->APBCMASK.reg |= PM_APBCMASK_TC4;
    GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5));
    while (GCLK->STATUS.bit.SYNCBUSY) {}
    TC4->COUNT16.CTRLA.bit.ENABLE = 0;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY) {}
    TC4->COUNT16.INTENCLR.reg = TC_INTENCLR_MC0 | TC_INTENCLR_MC1;
    TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0 | TC_INTFLAG_MC1;
    TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_MFRQ;
    uint8_t prescIdx = 0;
    uint32_t perVal = F_CPU_HZ / (matchHz * TC4_PRESCALER_DIV[prescIdx]);
    while (perVal > 65535u && prescIdx < TC4_PRESCALER_DIV_COUNT - 1) {
        prescIdx++;
        perVal = F_CPU_HZ / (matchHz * TC4_PRESCALER_DIV[prescIdx]);
    }
    if (perVal > 65535u) perVal = 65535u;
    if (perVal == 0u) perVal = 1u;
    TC4->COUNT16.CTRLA.bit.PRESCALER = prescIdx;
    const uint16_t perReg = (uint16_t)(perVal - 1u);
    TC4->COUNT16.CC[0].reg = perReg;
    if (envelopeWithDuty) {
        TC4->COUNT16.CC[1].reg = (uint16_t)(((uint32_t)perReg * (uint32_t)dutyPercent) / 100u);
        TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0 | TC_INTENSET_MC1;
    } else {
        TC4->COUNT16.CC[1].reg = 0;
        TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;
    }
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY) {}
    NVIC_EnableIRQ(TC4_IRQn);
    NVIC_SetPriority(TC4_IRQn, 1);
    TC4->COUNT16.CTRLA.bit.ENABLE = 1;
    while (TC4->COUNT16.STATUS.bit.SYNCBUSY) {}
}

void LedDriver::setOff() {
    sinusMode_ = false;
    useCarrier_ = false;
    NVIC_DisableIRQ(TC4_IRQn);
    TCC1->CTRLA.bit.ENABLE = 0;
    syncTCC(TCC1);
    digitalWrite(PIN_LED_OUTPUT_A, LOW);
    digitalWrite(PIN_LED_OUTPUT_B, LOW);
}

void LedDriver::setConstant(uint8_t intensityPercent) {
    sinusMode_ = false;
    NVIC_DisableIRQ(TC4_IRQn);
    useCarrier_ = false;
    setupTcc1CarrierPwm();
    /* CC sets off-time for this pinmux polarity */
    uint32_t cc = (carrierPer_ * (100u - (uint32_t)intensityPercent)) / 100u;
    setTcc1CompareBoth(cc);
    setTcc1Enabled(true);
}

void LedDriver::setFlicker(uint32_t freqHz, uint8_t dutyPercent, uint8_t intensityPercent) {
    sinusMode_ = false;
    flickerIntensityPercent_ = intensityPercent;
    useCarrier_ = true;
    NVIC_DisableIRQ(TC4_IRQn);
    setupTcc1CarrierPwm();
    setTcc1CompareBoth(0);
    setTcc1Enabled(true);
    configureTc4AtRate(freqHz, true, dutyPercent);
}

void LedDriver::setSinus(uint32_t freqHz, uint8_t intensityPercent) {
    NVIC_DisableIRQ(TC4_IRQn);
    useCarrier_ = true;
    sinusMode_ = true;
    flickerIntensityPercent_ = intensityPercent;
    setupTcc1CarrierPwm();
    setTcc1CompareBoth(0);
    setTcc1Enabled(true);
    sinusIndex_ = 0;
    const uint32_t isrFreq = freqHz * (uint32_t)SINE_TABLE_SIZE;
    configureTc4AtRate(isrFreq, false, 0);
}

/* ISR: avoid SYNCBUSY wait — write CC registers directly */
void LedDriver::sinusTimerIsr() {
    LedDriver& d = *instance_;
    if (d.carrierPer_ == 0) return;
    uint16_t raw = sineTable[d.sinusIndex_];
    d.sinusIndex_++;
    if (d.sinusIndex_ >= SINE_TABLE_SIZE) d.sinusIndex_ = 0;
    uint32_t onCc = ((uint32_t)raw * (uint32_t)d.flickerIntensityPercent_ * d.carrierPer_) / (100u * 4095u);
    uint32_t cc = (onCc >= d.carrierPer_) ? 0u : (d.carrierPer_ - onCc);
    TCC1->CC[0].reg = cc;
    TCC1->CC[1].reg = cc;
    TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
}

void LedDriver::flickerTimerIsr() {
    LedDriver& d = *instance_;
    if (!d.useCarrier_ || d.carrierPer_ == 0) return;
    uint32_t cc;
    if (TC4->COUNT16.INTFLAG.bit.MC0) {
        TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;
        cc = (d.carrierPer_ * (100u - (uint32_t)d.flickerIntensityPercent_)) / 100u;
    } else {
        TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC1;
        cc = d.carrierPer_;
    }
    TCC1->CC[0].reg = cc;
    TCC1->CC[1].reg = cc;
}

void TC4_Handler(void) {
    if (LedDriver::instance_ == nullptr) return;
    if (LedDriver::instance_->sinusMode_)
        LedDriver::sinusTimerIsr();
    else
        LedDriver::flickerTimerIsr();
}
