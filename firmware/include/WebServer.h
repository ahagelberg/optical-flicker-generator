#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <Ethernet.h>

class FlickerController;
class ConfigStore;

class WebServer {
public:
    WebServer(FlickerController& flicker, ConfigStore& config);
    void begin();
    void poll();
private:
    void serveClient(EthernetClient& client);
    void sendControlPage(EthernetClient& client);
    void sendConfigPage(EthernetClient& client);
    void handleApi(EthernetClient& client, const char* method, const char* path, const char* body);
    void sendJsonResponse(EthernetClient& client, int code, const char* body);
    void parseConfigPost(const char* body);
    FlickerController& flicker_;
    ConfigStore& config_;
    EthernetServer server_;
};

#endif
