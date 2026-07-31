#include "EthernetStatus.h"
#include "DebugLog.h"
#include "NetworkFormat.h"
#include <Ethernet.h>

static bool present_ = false;
static bool up_ = false;
static bool loggedNoHardware_ = false;
static EthernetHardwareStatus hardware_ = EthernetNoHardware;
static EthernetLinkStatus link_ = Unknown;
static IPAddress ip_;

void ethernetPoll() {
    hardware_ = Ethernet.hardwareStatus();
    present_ = (hardware_ != EthernetNoHardware);

    if (!present_) {
        if (!loggedNoHardware_) {
            debugLog("ethernet no hardware");
            loggedNoHardware_ = true;
        }
        link_ = Unknown;
        ip_ = IPAddress();
        if (up_) {
            debugLog("ethernet down");
            up_ = false;
        }
        return;
    }
    loggedNoHardware_ = false;

    link_ = Ethernet.linkStatus();
    ip_ = Ethernet.localIP();

    const bool hasIp = !(ip_[0] == 0 && ip_[1] == 0 && ip_[2] == 0 && ip_[3] == 0);
    const bool ready = (link_ == LinkON) && hasIp;

    if (ready == up_)
        return;

    up_ = ready;
    if (up_) {
        char ipStr[IPV4_STRING_BUFFER_LEN];
        formatIpv4(ip_, ipStr, sizeof(ipStr));
        debugLogf("ethernet up ip=%s", ipStr);
        return;
    }
    if (link_ != LinkON)
        debugLog("ethernet down (no link)");
    else
        debugLog("ethernet down (no ip)");
}

bool ethernetIsUp() {
    return up_;
}

bool ethernetHardwarePresent() {
    return present_;
}

IPAddress ethernetLocalIp() {
    return ip_;
}

const char* ethernetHardwareLabel() {
    switch (hardware_) {
        case EthernetW5100: return "W5100";
        case EthernetW5200: return "W5200";
        case EthernetW5500: return "W5500";
        default:            return "none";
    }
}

const char* ethernetLinkLabel() {
    switch (link_) {
        case LinkON:  return "on";
        case LinkOFF: return "off";
        default:      return "unknown";
    }
}
