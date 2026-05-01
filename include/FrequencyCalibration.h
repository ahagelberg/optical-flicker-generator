#ifndef FREQUENCY_CALIBRATION_H
#define FREQUENCY_CALIBRATION_H

#include <Arduino.h>
#include "ConfigStore.h"

/* Owns the in-memory calibration table, interpolation math, and persistence
   coordination (delegates byte layout to ConfigStore). */
class FrequencyCalibration {
public:
    FrequencyCalibration();
    FrequencyCalibration(const FrequencyCalibration&) = delete;
    FrequencyCalibration& operator=(const FrequencyCalibration&) = delete;

    void clear();
    /* Returns false if table is full (CAL_MAX_POINTS reached). */
    bool addPoint(uint32_t cmdHz, uint32_t measHz);
    uint8_t getCount() const { return count_; }

    /* Returns corrected command Hz for desired physical Hz.
       Returns desiredHz unchanged (identity) when fewer than 2 points are stored. */
    uint32_t commandHzForDesired(uint32_t desiredHz) const;

    /* Writes the data portion of the calibration table into buf, e.g. "0" or "8 1:1;5:5;...". */
    void writeTableString(char* buf, size_t len) const;

    void load(const ConfigStore::CalibrationData& data);
    /* Writes current table into ConfigStore and calls store.save(). */
    void save(ConfigStore& store) const;

private:
    uint32_t cmdHz_[CAL_MAX_POINTS];
    uint32_t measHz_[CAL_MAX_POINTS];
    uint8_t count_;
};

#endif
