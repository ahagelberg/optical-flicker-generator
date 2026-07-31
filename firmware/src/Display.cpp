#include "Display.h"
#include "ConfigStore.h"
#include "FlickerController.h"
#include "Config.h"
#include "EthernetStatus.h"
#include "NetworkFormat.h"
#include "DebugLog.h"
#include <U8g2lib.h>

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

static const uint8_t DISPLAY_LABEL_COL_X = 0;
static const uint8_t DISPLAY_VALUE_COL_X = 36;
static const uint8_t DISPLAY_INTENSITY_COL_X = 96;
static const uint8_t DISPLAY_INT_LABEL_COL_X = 72;
static const uint8_t DISPLAY_ROW_MODE_Y = 10;
static const uint8_t DISPLAY_ROW_FREQ_Y = 24;
static const uint8_t DISPLAY_ROW_DUTY_INT_Y = 38;
static const uint8_t DISPLAY_ROW_IP_Y = 52;
static const uint8_t DISPLAY_IP_VALUE_X = 24;

static const uint8_t DISPLAY_MODE_STR_X = 36;
static const uint8_t DISPLAY_MODE_STR_Y = 10;
static const uint8_t DISPLAY_MODE_BOX_X = 34;
static const uint8_t DISPLAY_MODE_BOX_Y = 0;
static const uint8_t DISPLAY_MODE_BOX_W = 128 - DISPLAY_MODE_BOX_X;
static const uint8_t DISPLAY_MODE_BOX_H = 12;

static const char DISPLAY_BOOT_MESSAGE[] = "Init...";
static const uint8_t DISPLAY_BOOT_MSG_X = 40;
static const uint8_t DISPLAY_BOOT_MSG_Y = 32;

/* SSD1306 common 7-bit addresses. U8g2 wants the 8-bit write address (<< 1). */
static const uint8_t DISPLAY_I2C_ADDR_7BIT_PRIMARY = 0x3C;
static const uint8_t DISPLAY_I2C_ADDR_7BIT_ALT = 0x3D;
static const unsigned DISPLAY_I2C_BIT_DELAY_US = 5;
static const unsigned DISPLAY_I2C_CLOCK_STRETCH_TIMEOUT_US = 1000;
static const uint8_t DISPLAY_I2C_BUS_RECOVER_CLOCKS = 9;

Display::Display(FlickerController& flicker, ConfigStore& config)
    : flicker_(flicker), config_(config), lastUpdateMs_(0), lastUserActivityMs_(0),
      screensaverOn_(false), prevButtonPressed_(false), present_(false) {}

static void displaySclHigh() {
    pinMode(SCL, INPUT_PULLUP);
}

static void displaySclLow() {
    pinMode(SCL, OUTPUT);
    digitalWrite(SCL, LOW);
}

static void displaySdaHigh() {
    pinMode(SDA, INPUT_PULLUP);
}

static void displaySdaLow() {
    pinMode(SDA, OUTPUT);
    digitalWrite(SDA, LOW);
}

static bool displayWaitSclHigh() {
    displaySclHigh();
    const unsigned long startUs = micros();
    while (digitalRead(SCL) == LOW) {
        if ((micros() - startUs) > DISPLAY_I2C_CLOCK_STRETCH_TIMEOUT_US)
            return false;
    }
    return true;
}

static bool displayI2cWriteBit(bool one) {
    if (one)
        displaySdaHigh();
    else
        displaySdaLow();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    if (!displayWaitSclHigh())
        return false;
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    displaySclLow();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    return true;
}

/* Timed bit-bang probe — never uses SERCOM Wire (which can wait forever on a stuck bus). */
static bool displayI2cProbe(uint8_t addr7bit) {
    displaySdaHigh();
    displaySclHigh();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);

    if (digitalRead(SDA) == LOW || digitalRead(SCL) == LOW) {
        for (uint8_t i = 0; i < DISPLAY_I2C_BUS_RECOVER_CLOCKS; i++) {
            displaySclLow();
            delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
            if (!displayWaitSclHigh())
                return false;
            delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
        }
        displaySdaLow();
        delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
        if (!displayWaitSclHigh())
            return false;
        delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
        displaySdaHigh();
        delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
        if (digitalRead(SDA) == LOW || digitalRead(SCL) == LOW)
            return false;
    }

    /* START */
    displaySdaLow();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    displaySclLow();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);

    const uint8_t addrByte = (uint8_t)((addr7bit << 1) | 0u); /* write */
    for (int8_t bit = 7; bit >= 0; bit--) {
        if (!displayI2cWriteBit((addrByte >> bit) & 0x1))
            return false;
    }

    /* ACK from slave pulls SDA low */
    displaySdaHigh();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    if (!displayWaitSclHigh())
        return false;
    const bool ack = (digitalRead(SDA) == LOW);
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    displaySclLow();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);

    /* STOP */
    displaySdaLow();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    if (!displayWaitSclHigh())
        return false;
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    displaySdaHigh();
    delayMicroseconds(DISPLAY_I2C_BIT_DELAY_US);
    return ack;
}

void Display::showBootSplash() {
    uint8_t addr7 = 0;
    if (displayI2cProbe(DISPLAY_I2C_ADDR_7BIT_PRIMARY))
        addr7 = DISPLAY_I2C_ADDR_7BIT_PRIMARY;
    else if (displayI2cProbe(DISPLAY_I2C_ADDR_7BIT_ALT))
        addr7 = DISPLAY_I2C_ADDR_7BIT_ALT;

    if (addr7 == 0) {
        present_ = false;
        debugLog("display not found");
        return;
    }

    present_ = true;
    u8g2.setI2CAddress((uint8_t)(addr7 << 1));
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB08_tf);
    u8g2.drawStr(DISPLAY_BOOT_MSG_X, DISPLAY_BOOT_MSG_Y, DISPLAY_BOOT_MESSAGE);
    u8g2.sendBuffer();
}

void Display::begin() {
    if (!present_)
        return;
    u8g2.setPowerSave(0);
    screensaverOn_ = false;
    unsigned long t = millis();
    lastUserActivityMs_ = t;
    lastUpdateMs_ = 0;
    prevButtonPressed_ = (digitalRead(PIN_RESET_BUTTON) == LOW);
}

void Display::refreshFullRedraw(unsigned long now) {
    if (!present_)
        return;
    char buf[IPV4_STRING_BUFFER_LEN];
    const FlickerMode mode = flicker_.getMode();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvR08_tf);
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_MODE_Y, "Mode:");
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_FREQ_Y, "Freq:");
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_DUTY_INT_Y, "Duty:");
    u8g2.drawStr(DISPLAY_INT_LABEL_COL_X, DISPLAY_ROW_DUTY_INT_Y, "Int:");
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_IP_Y, "IP:");
    u8g2.setFont(u8g2_font_helvB08_tf);
    if (mode != FlickerMode::Off) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(DISPLAY_MODE_BOX_X, DISPLAY_MODE_BOX_Y, DISPLAY_MODE_BOX_W, DISPLAY_MODE_BOX_H);
        u8g2.setDrawColor(0);
        u8g2.drawStr(DISPLAY_MODE_STR_X, DISPLAY_MODE_STR_Y, flicker_.getModeString());
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(DISPLAY_MODE_STR_X, DISPLAY_MODE_STR_Y, flicker_.getModeString());
    }
    if (mode == FlickerMode::Constant) {
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_FREQ_Y, "N/A");
    } else {
        snprintf(buf, sizeof(buf), "%lu Hz", (unsigned long)flicker_.getFrequencyHz());
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_FREQ_Y, buf);
    }
    if (mode == FlickerMode::Sinus || mode == FlickerMode::Constant) {
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_DUTY_INT_Y, "N/A");
    } else {
        snprintf(buf, sizeof(buf), "%u%%", flicker_.getDutyPercent());
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_DUTY_INT_Y, buf);
    }
    snprintf(buf, sizeof(buf), "%u%%", flicker_.getIntensityPercent());
    u8g2.drawStr(DISPLAY_INTENSITY_COL_X, DISPLAY_ROW_DUTY_INT_Y, buf);
    if (ethernetIsUp()) {
        formatIpv4(ethernetLocalIp(), buf, sizeof(buf));
        u8g2.drawStr(DISPLAY_IP_VALUE_X, DISPLAY_ROW_IP_Y, buf);
    } else {
        u8g2.drawStr(DISPLAY_IP_VALUE_X, DISPLAY_ROW_IP_Y, "--");
    }
    u8g2.sendBuffer();
    lastUpdateMs_ = now;
}

void Display::update() {
    if (!present_)
        return;

    unsigned long now = millis();
    const bool pressed = (digitalRead(PIN_RESET_BUTTON) == LOW);
    const bool pressEdge = pressed && !prevButtonPressed_;
    prevButtonPressed_ = pressed;

    if (screensaverOn_) {
        if (!pressEdge)
            return;
        screensaverOn_ = false;
        u8g2.setPowerSave(0);
        lastUserActivityMs_ = now;
        refreshFullRedraw(now);
        return;
    }

    if (pressEdge)
        lastUserActivityMs_ = now;

    const unsigned long idleMs = (unsigned long)config_.getScreensaverTimeoutS() * 1000UL;
    if ((unsigned long)(now - lastUserActivityMs_) >= idleMs) {
        screensaverOn_ = true;
        u8g2.setPowerSave(1);
        return;
    }

    if ((unsigned long)(now - lastUpdateMs_) < DISPLAY_UPDATE_MS)
        return;
    refreshFullRedraw(now);
}
