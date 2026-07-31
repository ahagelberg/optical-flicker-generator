#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>
#include "Config.h"

class ConfigStore {
public:
    /* POD struct for calibration data exchange; owned by ConfigStore for layout, used by FrequencyCalibration for math. */
    struct CalibrationData {
        uint8_t count;
        uint32_t cmdHz[CAL_MAX_POINTS];
        uint32_t measHz[CAL_MAX_POINTS];
    };

    ConfigStore();
    ConfigStore(const ConfigStore&) = delete;
    ConfigStore& operator=(const ConfigStore&) = delete;

    void load();
    void save();
    void resetToDefaults();

    bool getUseDhcp() const { return useDhcp_; }
    void setUseDhcp(bool use) { useDhcp_ = use; }
    void getIp(uint8_t* out) const;
    void setIp(const uint8_t* ip);
    void getGateway(uint8_t* out) const;
    void setGateway(const uint8_t* gw);
    void getSubnet(uint8_t* out) const;
    void setSubnet(const uint8_t* sn);
    uint32_t getCarrierHz() const { return carrierHz_; }
    void setCarrierHz(uint32_t hz);
    uint16_t getScreensaverTimeoutS() const { return screensaverTimeoutS_; }
    void setScreensaverTimeoutS(uint16_t seconds);

    CalibrationData getCalibrationData() const;
    void setCalibrationData(const CalibrationData& data);

private:
    /* Bump magic when StoredConfig layout changes to trigger migration to defaults. */
    static const uint32_t CONFIG_MAGIC = 0x464C4354;  /* "FLCS" v3: adds screensaver timeout */
    struct StoredConfig {
        uint32_t magic;
        bool useDhcp;
        uint8_t ip[4];
        uint8_t gateway[4];
        uint8_t subnet[4];
        uint32_t carrierHz;
        uint16_t screensaverTimeoutS;
        uint8_t calCount;
        uint32_t calCmdHz[CAL_MAX_POINTS];
        uint32_t calMeasHz[CAL_MAX_POINTS];
    };

    bool useDhcp_;
    uint8_t ip_[4];
    uint8_t gateway_[4];
    uint8_t subnet_[4];
    uint32_t carrierHz_;
    uint16_t screensaverTimeoutS_;
    CalibrationData cal_;

    void applyStored(const StoredConfig& c);
    void toStored(StoredConfig& c) const;
};

#endif
