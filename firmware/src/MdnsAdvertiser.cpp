#include "MdnsAdvertiser.h"
#include "Config.h"
#include "DeviceInfo.h"
#include "DebugLog.h"
#include "EthernetStatus.h"
#include "NetworkFormat.h"
#include <EthernetUdp.h>
#include <ArduinoMDNS.h>

static EthernetUDP mdnsUdp;
static MDNS mdns(mdnsUdp);
static char mdnsHostname[MDNS_HOSTNAME_BUFFER_LEN];

MdnsAdvertiser::MdnsAdvertiser() : started_(false), announcedIp_() {}

void MdnsAdvertiser::stop() {
    if (!started_)
        return;
    mdns.removeAllServiceRecords();
    mdnsUdp.stop();
    started_ = false;
    announcedIp_ = IPAddress();
    debugLog("mdns stopped");
}

void MdnsAdvertiser::startAdvertising(const IPAddress& ip) {
    if (started_) {
        mdns.removeAllServiceRecords();
        mdnsUdp.stop();
        started_ = false;
    }
    DeviceInfo::writeMdnsHostname(mdnsHostname, sizeof(mdnsHostname));
    if (!mdns.begin(ip, mdnsHostname)) {
        debugLog("mdns begin failed");
        return;
    }
    mdns.addServiceRecord(MDNS_HTTP_SERVICE_NAME, HTTP_PORT, MDNSServiceTCP);
    announcedIp_ = ip;
    started_ = true;
    char ipStr[IPV4_STRING_BUFFER_LEN];
    formatIpv4(ip, ipStr, sizeof(ipStr));
    debugLogf("mdns hostname=%s.local ip=%s", mdnsHostname, ipStr);
}

void MdnsAdvertiser::poll() {
    const IPAddress ip = ethernetLocalIp();
    if (!started_ || ip != announcedIp_) {
        startAdvertising(ip);
        return;
    }
    mdns.run();
}
