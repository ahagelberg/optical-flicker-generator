#include <Arduino.h>
#include "Config.h"
#include "ConfigStore.h"
#include "FrequencyCalibration.h"
#include "LedDriver.h"
#include "FlickerController.h"
#include "CommandShell.h"
#include "SerialCommand.h"
#include "Display.h"
#include "SocketServer.h"
#include "WebServer.h"
#include "ResetButton.h"
#include "MdnsAdvertiser.h"
#include "DebugLog.h"
#include "DeviceInfo.h"
#include "NetworkFormat.h"
#include "EthernetStatus.h"
#include "SerialTx.h"
#include <Ethernet.h>

ConfigStore configStore;
FrequencyCalibration freqCal;
LedDriver ledDriver;
FlickerController flickerController(ledDriver, freqCal);
CommandShell commandShell(flickerController, freqCal, configStore);
SerialCommand serialCommand(commandShell);
Display display(flickerController, configStore);
SocketServer socketServer(commandShell);
WebServer webServer(flickerController, configStore);
ResetButton resetButton(configStore);
MdnsAdvertiser mdnsAdvertiser;

void setup() {
    /* OLED first — never block on USB CDC before user-visible boot feedback. */
    display.showBootSplash();
    resetButton.begin();
    Serial.begin(115200);
    debugLog("booting...");
    configStore.load();
    ConfigStore::CalibrationData calData = configStore.getCalibrationData();
    freqCal.load(calData);
    flickerController.setCarrierHz(configStore.getCarrierHz());
    Ethernet.init(PIN_ETHERNET_CS);
    uint8_t mac[DEVICE_INFO_MAC_LEN];
    DeviceInfo::writeMacAddress(mac, sizeof(mac));
    if (configStore.getUseDhcp()) {
        Ethernet.begin(mac);
    } else {
        uint8_t ip[4], gw[4], sn[4];
        configStore.getIp(ip);
        configStore.getGateway(gw);
        configStore.getSubnet(sn);
        /* Fourth arg is DNS server; gateway is used until config stores DNS separately. */
        Ethernet.begin(mac, ip, gw, gw, sn);
    }
    ethernetPoll();
    char idHex[DEVICE_INFO_ID_HEX_BUFFER_LEN];
    DeviceInfo::writeDeviceIdHex(idHex, sizeof(idHex));
    debugLogf("boot %s %s id=%s",
              DeviceInfo::deviceType(), DeviceInfo::firmwareVersion(), idHex);
    {
        char macStr[DEVICE_INFO_MAC_STRING_BUFFER_LEN];
        DeviceInfo::writeMacAddressString(macStr, sizeof(macStr));
        debugLogf("ethernet mac=%s", macStr);
    }
    debugLogf("config dhcp=%u carrier=%lu Hz screensaver=%u s cal_points=%u",
              (unsigned)configStore.getUseDhcp(),
              (unsigned long)configStore.getCarrierHz(),
              (unsigned)configStore.getScreensaverTimeoutS(),
              (unsigned)calData.count);
    {
        char ipStr[IPV4_STRING_BUFFER_LEN];
        formatIpv4(ethernetLocalIp(), ipStr, sizeof(ipStr));
        debugLogf("ethernet ip=%s hardware=%s link=%s",
                  ipStr, ethernetHardwareLabel(), ethernetLinkLabel());
    }
    flickerController.begin();
    display.begin();
    if (ethernetHardwarePresent()) {
        socketServer.begin();
        webServer.begin();
    }
    debugLog("boot complete");
}

void loop() {
    serialCommand.poll();
    serialTxPoll();
    ethernetPoll();
    static bool networkServicesActive = false;
    if (ethernetIsUp()) {
        socketServer.poll();
        webServer.poll();
        mdnsAdvertiser.poll();
        networkServicesActive = true;
    } else if (networkServicesActive) {
        socketServer.stopClient();
        mdnsAdvertiser.stop();
        networkServicesActive = false;
    }
    display.update();
    resetButton.poll();
}
