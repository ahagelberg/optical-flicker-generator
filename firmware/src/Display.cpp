#include "Display.h"
#include "ConfigStore.h"
#include "FlickerController.h"
#include "Config.h"
#include "NetworkFormat.h"
#include <Ethernet.h>
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

Display::Display(FlickerController& flicker, ConfigStore& config)
    : flicker_(flicker), config_(config), lastUpdateMs_(0), lastUserActivityMs_(0),
      screensaverOn_(false), prevButtonPressed_(false) {}

void Display::showBootSplash() {
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB08_tf);
    u8g2.drawStr(DISPLAY_BOOT_MSG_X, DISPLAY_BOOT_MSG_Y, DISPLAY_BOOT_MESSAGE);
    u8g2.sendBuffer();
}

void Display::begin() {
    u8g2.setPowerSave(0);
    screensaverOn_ = false;
    unsigned long t = millis();
    lastUserActivityMs_ = t;
    lastUpdateMs_ = 0;
    prevButtonPressed_ = (digitalRead(PIN_RESET_BUTTON) == LOW);
}

void Display::refreshFullRedraw(unsigned long now) {
    char buf[IPV4_STRING_BUFFER_LEN];
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvR08_tf);
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_MODE_Y, "Mode:");
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_FREQ_Y, "Freq:");
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_DUTY_INT_Y, "Duty:");
    u8g2.drawStr(DISPLAY_INT_LABEL_COL_X, DISPLAY_ROW_DUTY_INT_Y, "Int:");
    u8g2.drawStr(DISPLAY_LABEL_COL_X, DISPLAY_ROW_IP_Y, "IP:");
    u8g2.setFont(u8g2_font_helvB08_tf);
    if (flicker_.getMode() != FlickerMode::Off) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(DISPLAY_MODE_BOX_X, DISPLAY_MODE_BOX_Y, DISPLAY_MODE_BOX_W, DISPLAY_MODE_BOX_H);
        u8g2.setDrawColor(0);
        u8g2.drawStr(DISPLAY_MODE_STR_X, DISPLAY_MODE_STR_Y, flicker_.getModeString());
        u8g2.setDrawColor(1);
    } else {
        u8g2.drawStr(DISPLAY_MODE_STR_X, DISPLAY_MODE_STR_Y, flicker_.getModeString());
    }
    if (flicker_.getMode() == FlickerMode::Constant) {
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_FREQ_Y, "N/A");
    } else {
        snprintf(buf, sizeof(buf), "%lu Hz", (unsigned long)flicker_.getFrequencyHz());
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_FREQ_Y, buf);
    }
    if (flicker_.getMode() == FlickerMode::Sinus || flicker_.getMode() == FlickerMode::Constant) {
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_DUTY_INT_Y, "N/A");
    } else {
        snprintf(buf, sizeof(buf), "%u%%", flicker_.getDutyPercent());
        u8g2.drawStr(DISPLAY_VALUE_COL_X, DISPLAY_ROW_DUTY_INT_Y, buf);
    }
    snprintf(buf, sizeof(buf), "%u%%", flicker_.getIntensityPercent());
    u8g2.drawStr(DISPLAY_INTENSITY_COL_X, DISPLAY_ROW_DUTY_INT_Y, buf);
    formatIpv4(Ethernet.localIP(), buf, sizeof(buf));
    u8g2.drawStr(DISPLAY_IP_VALUE_X, DISPLAY_ROW_IP_Y, buf[0] ? buf : "--");
    u8g2.sendBuffer();
    lastUpdateMs_ = now;
}

void Display::update() {
    unsigned long now = millis();
    const bool pressed = (digitalRead(PIN_RESET_BUTTON) == LOW);
    const bool pressEdge = pressed && !prevButtonPressed_;
    prevButtonPressed_ = pressed;

    if (screensaverOn_) {
        if (pressEdge) {
            screensaverOn_ = false;
            u8g2.setPowerSave(0);
            lastUserActivityMs_ = now;
            refreshFullRedraw(now);
        }
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

    if ((unsigned long)(now - lastUpdateMs_) < DISPLAY_UPDATE_MS) return;
    refreshFullRedraw(now);
}
