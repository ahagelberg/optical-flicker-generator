#ifndef MDNS_ADVERTISER_H
#define MDNS_ADVERTISER_H

#include <Arduino.h>

class MdnsAdvertiser {
public:
    MdnsAdvertiser();
    void begin();
    void poll();
private:
    void startAdvertising(const IPAddress& ip);
    bool started_;
    IPAddress announcedIp_;
};

#endif
