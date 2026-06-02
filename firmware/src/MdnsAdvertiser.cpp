#include "MdnsAdvertiser.h"
#include "Config.h"
#include "DeviceInfo.h"
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <ArduinoMDNS.h>

static EthernetUDP mdnsUdp;
static MDNS mdns(mdnsUdp);
static char mdnsHostname[MDNS_HOSTNAME_BUFFER_LEN];

MdnsAdvertiser::MdnsAdvertiser() : started_(false), announcedIp_() {}

void MdnsAdvertiser::startAdvertising(const IPAddress& ip) {
    if (started_) {
        mdns.removeAllServiceRecords();
        mdnsUdp.stop();
    }
    DeviceInfo::writeMdnsHostname(mdnsHostname, sizeof(mdnsHostname));
    if (!mdns.begin(ip, mdnsHostname))
        return;
    mdns.addServiceRecord(MDNS_HTTP_SERVICE_NAME, HTTP_PORT, MDNSServiceTCP);
    announcedIp_ = ip;
    started_ = true;
}

void MdnsAdvertiser::begin() {
    const IPAddress ip = Ethernet.localIP();
    if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0)
        return;
    startAdvertising(ip);
}

void MdnsAdvertiser::poll() {
    if (!started_)
        begin();
    const IPAddress ip = Ethernet.localIP();
    if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) {
        if (started_) {
            mdns.removeAllServiceRecords();
            mdnsUdp.stop();
            started_ = false;
            announcedIp_ = IPAddress();
        }
        return;
    }
    if (ip != announcedIp_)
        startAdvertising(ip);
    if (started_)
        mdns.run();
}
