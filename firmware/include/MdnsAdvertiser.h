#ifndef MDNS_ADVERTISER_H
#define MDNS_ADVERTISER_H

#include <Arduino.h>
#include <IPAddress.h>

class MdnsAdvertiser {
public:
    MdnsAdvertiser();
    void poll();
    void stop();
private:
    void startAdvertising(const IPAddress& ip);
    bool started_;
    IPAddress announcedIp_;
};

#endif
