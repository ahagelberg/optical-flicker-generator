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
#include <Ethernet.h>

static const uint8_t MAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

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
    display.showBootSplash();
    configStore.load();
    freqCal.load(configStore.getCalibrationData());
    flickerController.setCarrierHz(configStore.getCarrierHz());
    Ethernet.init(PIN_ETHERNET_CS);
    if (configStore.getUseDhcp()) {
        Ethernet.begin((uint8_t*)MAC);
    } else {
        uint8_t ip[4], gw[4], sn[4];
        configStore.getIp(ip);
        configStore.getGateway(gw);
        configStore.getSubnet(sn);
        /* Fourth arg is DNS server; gateway is used until config stores DNS separately. */
        Ethernet.begin((uint8_t*)MAC, ip, gw, gw, sn);
    }
    Serial.begin(115200);
    flickerController.begin();
    display.begin();
    socketServer.begin();
    webServer.begin();
    mdnsAdvertiser.begin();
    resetButton.begin();
}

void loop() {
    serialCommand.poll();
    socketServer.poll();
    webServer.poll();
    mdnsAdvertiser.poll();
    display.update();
    resetButton.poll();
}
