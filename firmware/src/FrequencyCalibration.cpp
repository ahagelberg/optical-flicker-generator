#include "FrequencyCalibration.h"
#include "Config.h"

FrequencyCalibration::FrequencyCalibration() : count_(0) {
    memset(cmdHz_, 0, sizeof(cmdHz_));
    memset(measHz_, 0, sizeof(measHz_));
}

void FrequencyCalibration::clear() {
    count_ = 0;
}

bool FrequencyCalibration::addPoint(uint32_t cmdHz, uint32_t measHz) {
    if (count_ >= CAL_MAX_POINTS) return false;
    cmdHz_[count_] = cmdHz;
    measHz_[count_] = measHz;
    count_++;
    return true;
}

uint32_t FrequencyCalibration::commandHzForDesired(uint32_t desiredHz) const {
    if (count_ < 2) return desiredHz;
    /* Piecewise-linear inverse interpolation: points are sorted by cmdHz (wizard nominal sequence).
       We traverse (measHz, cmdHz) pairs to map desired physical Hz → command Hz. */
    if (desiredHz <= measHz_[0]) return cmdHz_[0];
    if (desiredHz >= measHz_[count_ - 1]) return cmdHz_[count_ - 1];
    for (uint8_t i = 0; i < count_ - 1; i++) {
        if (desiredHz >= measHz_[i] && desiredHz <= measHz_[i + 1]) {
            uint32_t measSpan = measHz_[i + 1] - measHz_[i];
            if (measSpan == 0) return cmdHz_[i];
            uint32_t cmdSpan = cmdHz_[i + 1] - cmdHz_[i];
            return cmdHz_[i] + (uint32_t)((uint64_t)(desiredHz - measHz_[i]) * cmdSpan / measSpan);
        }
    }
    return desiredHz;
}

void FrequencyCalibration::writeTableString(char* buf, size_t len) const {
    if (count_ == 0) {
        snprintf(buf, len, "0");
        return;
    }
    int written = snprintf(buf, len, "%u", count_);
    for (uint8_t i = 0; i < count_ && written < (int)len - 1; i++) {
        written += snprintf(buf + written, len - (size_t)written,
                            i == 0 ? " %lu:%lu" : ";%lu:%lu",
                            (unsigned long)cmdHz_[i], (unsigned long)measHz_[i]);
    }
}

void FrequencyCalibration::load(const ConfigStore::CalibrationData& data) {
    count_ = data.count <= CAL_MAX_POINTS ? data.count : 0;
    for (uint8_t i = 0; i < count_; i++) {
        cmdHz_[i] = data.cmdHz[i];
        measHz_[i] = data.measHz[i];
    }
}

void FrequencyCalibration::save(ConfigStore& store) const {
    ConfigStore::CalibrationData data;
    data.count = count_;
    for (uint8_t i = 0; i < count_; i++) {
        data.cmdHz[i] = cmdHz_[i];
        data.measHz[i] = measHz_[i];
    }
    store.setCalibrationData(data);
    store.save();
}
