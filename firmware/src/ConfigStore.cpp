#include "ConfigStore.h"
#include "Config.h"
#include <FlashStorage_SAMD.h>

ConfigStore::ConfigStore()
    : useDhcp_(true), carrierHz_(DEFAULT_CARRIER_HZ),
      screensaverTimeoutS_(DEFAULT_DISPLAY_SCREENSAVER_S) {
    memset(ip_, 0, 4);
    memset(gateway_, 0, 4);
    memset(subnet_, 0, 4);
    cal_.count = 0;
    memset(cal_.cmdHz, 0, sizeof(cal_.cmdHz));
    memset(cal_.measHz, 0, sizeof(cal_.measHz));
}

void ConfigStore::load() {
    StoredConfig c;
    EEPROM.get(0, c);
    if (c.magic == CONFIG_MAGIC) {
        applyStored(c);
    } else {
        resetToDefaults();
    }
}

void ConfigStore::save() {
    StoredConfig c;
    toStored(c);
    EEPROM.put(0, c);
    EEPROM.commit();
}

void ConfigStore::resetToDefaults() {
    useDhcp_ = true;
    memset(ip_, 0, 4);
    memset(gateway_, 0, 4);
    memset(subnet_, 0, 4);
    carrierHz_ = DEFAULT_CARRIER_HZ;
    screensaverTimeoutS_ = DEFAULT_DISPLAY_SCREENSAVER_S;
    /* Calibration is intentionally preserved across factory reset — it is factory data,
       not user configuration. Clear it only by running the calibration wizard again. */
}

void ConfigStore::applyStored(const StoredConfig& c) {
    useDhcp_ = c.useDhcp;
    memcpy(ip_, c.ip, 4);
    memcpy(gateway_, c.gateway, 4);
    memcpy(subnet_, c.subnet, 4);
    carrierHz_ = (c.carrierHz >= MIN_CARRIER_HZ && c.carrierHz <= MAX_CARRIER_HZ)
        ? c.carrierHz : DEFAULT_CARRIER_HZ;
    screensaverTimeoutS_ = (c.screensaverTimeoutS >= MIN_DISPLAY_SCREENSAVER_S
            && c.screensaverTimeoutS <= MAX_DISPLAY_SCREENSAVER_S)
        ? c.screensaverTimeoutS : DEFAULT_DISPLAY_SCREENSAVER_S;
    cal_.count = c.calCount <= CAL_MAX_POINTS ? c.calCount : 0;
    for (uint8_t i = 0; i < cal_.count; i++) {
        cal_.cmdHz[i] = c.calCmdHz[i];
        cal_.measHz[i] = c.calMeasHz[i];
    }
}

void ConfigStore::toStored(StoredConfig& c) const {
    c.magic = CONFIG_MAGIC;
    c.useDhcp = useDhcp_;
    memcpy(c.ip, ip_, 4);
    memcpy(c.gateway, gateway_, 4);
    memcpy(c.subnet, subnet_, 4);
    c.carrierHz = carrierHz_;
    c.screensaverTimeoutS = screensaverTimeoutS_;
    c.calCount = cal_.count;
    for (uint8_t i = 0; i < cal_.count; i++) {
        c.calCmdHz[i] = cal_.cmdHz[i];
        c.calMeasHz[i] = cal_.measHz[i];
    }
}

void ConfigStore::getIp(uint8_t* out) const      { memcpy(out, ip_, 4); }
void ConfigStore::getGateway(uint8_t* out) const  { memcpy(out, gateway_, 4); }
void ConfigStore::getSubnet(uint8_t* out) const   { memcpy(out, subnet_, 4); }

void ConfigStore::setIp(const uint8_t* ip)       { if (ip) memcpy(ip_, ip, 4); }
void ConfigStore::setGateway(const uint8_t* gw)  { if (gw) memcpy(gateway_, gw, 4); }
void ConfigStore::setSubnet(const uint8_t* sn)   { if (sn) memcpy(subnet_, sn, 4); }

void ConfigStore::setCarrierHz(uint32_t hz) {
    if (hz < MIN_CARRIER_HZ) hz = MIN_CARRIER_HZ;
    if (hz > MAX_CARRIER_HZ) hz = MAX_CARRIER_HZ;
    carrierHz_ = hz;
}

void ConfigStore::setScreensaverTimeoutS(uint16_t seconds) {
    if (seconds < MIN_DISPLAY_SCREENSAVER_S) seconds = MIN_DISPLAY_SCREENSAVER_S;
    if (seconds > MAX_DISPLAY_SCREENSAVER_S) seconds = MAX_DISPLAY_SCREENSAVER_S;
    screensaverTimeoutS_ = seconds;
}

ConfigStore::CalibrationData ConfigStore::getCalibrationData() const {
    return cal_;
}

void ConfigStore::setCalibrationData(const CalibrationData& data) {
    cal_ = data;
}
