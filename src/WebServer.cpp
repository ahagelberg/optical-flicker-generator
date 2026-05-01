#include "WebServer.h"
#include "FlickerController.h"
#include "ConfigStore.h"
#include "Config.h"
#include "DeviceInfo.h"
#include "NetworkFormat.h"
#include "favicon_data.h"
#include <Ethernet.h>

static const char API_PREFIX[] = "/api/";
static const size_t API_PREFIX_LEN = 5;

static const size_t HTTP_METHOD_BUF_LEN = 8;
static const size_t HTTP_PATH_BUF_LEN = 64;
static const size_t HTTP_API_IDENTIFY_JSON_CAP = 192;

static const size_t FAVICON_SEND_CHUNK = 512;

static bool jsonUintAfterKey(const char* body, const char* key, unsigned int* out) {
    /* Search for "key": as a unit so the closing quote and colon are consumed. */
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char* p = strstr(body, pattern);
    if (!p) return false;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t') p++;
    return sscanf(p, "%u", out) == 1;
}

static void parseAndApplyBody(FlickerController& flicker, const char* body) {
    if (!body || !body[0]) return;
    unsigned int v;
    if (jsonUintAfterKey(body, "frequency", &v))
        flicker.setFrequency((uint32_t)v);
    if (jsonUintAfterKey(body, "dutycycle", &v))
        flicker.setDutyCycle((uint8_t)v);
    if (jsonUintAfterKey(body, "intensity", &v))
        flicker.setIntensity((uint8_t)v);
}

static void sendFaviconLink(EthernetClient& client) {
    client.println("<link rel=\"icon\" type=\"image/png\" href=\"/favicon.ico\">");
}

static void sendFaviconResponse(EthernetClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/png");
    client.print("Content-Length: ");
    client.println(FAVICON_PNG_LEN);
    client.println("Connection: close");
    client.println();
    for (unsigned int i = 0; i < FAVICON_PNG_LEN; i += FAVICON_SEND_CHUNK) {
        unsigned int n = FAVICON_PNG_LEN - i;
        if (n > FAVICON_SEND_CHUNK) n = FAVICON_SEND_CHUNK;
        client.write(FAVICON_PNG + i, n);
    }
    client.flush();
}

WebServer::WebServer(FlickerController& flicker, ConfigStore& config)
    : flicker_(flicker), config_(config), server_(HTTP_PORT) {}

void WebServer::begin() {
    server_.begin();
}

void WebServer::poll() {
    EthernetClient client = server_.accept();
    if (client) {
        serveClient(client);
        client.stop();
    }
}

static void skipToSpace(const char*& p) {
    while (*p && *p != ' ' && *p != '\r' && *p != '\n') p++;
}
static void skipSpaces(const char*& p) {
    while (*p == ' ') p++;
}

void WebServer::serveClient(EthernetClient& client) {
    String reqLine = client.readStringUntil('\n');
    reqLine.trim();
    const char* p = reqLine.c_str();
    const char* method = p;
    skipToSpace(p);
    char methodBuf[HTTP_METHOD_BUF_LEN];
    size_t methodLen = (size_t)(p - method);
    if (methodLen >= sizeof(methodBuf)) methodLen = sizeof(methodBuf) - 1;
    memcpy(methodBuf, method, methodLen);
    methodBuf[methodLen] = '\0';
    skipSpaces(p);
    const char* path = p;
    skipToSpace(p);
    char pathBuf[HTTP_PATH_BUF_LEN];
    size_t pathLen = (size_t)(p - path);
    if (pathLen >= sizeof(pathBuf)) pathLen = sizeof(pathBuf) - 1;
    memcpy(pathBuf, path, pathLen);
    pathBuf[pathLen] = '\0';
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.length() == 0 || line == "\r") break;
    }
    String body;
    while (client.available()) body += (char)client.read();
    if (strncmp(pathBuf, API_PREFIX, API_PREFIX_LEN) == 0) {
        handleApi(client, methodBuf, pathBuf + API_PREFIX_LEN, body.c_str());
        return;
    }
    if (strcmp(pathBuf, "/config") == 0) {
        if (strcmp(methodBuf, "POST") == 0 && body.length() > 0) {
            parseConfigPost(body.c_str());
            config_.save();
        }
        sendConfigPage(client);
        return;
    }
    if (strcmp(methodBuf, "GET") == 0 &&
        (strcmp(pathBuf, "/favicon.ico") == 0 || strcmp(pathBuf, "/favicon.png") == 0)) {
        sendFaviconResponse(client);
        return;
    }
    if (strcmp(pathBuf, "/") == 0) {
        sendControlPage(client);
        return;
    }
    sendJsonResponse(client, 404, "{\"error\":\"Not found\"}");
}

void WebServer::sendControlPage(EthernetClient& client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><title>Flicker Control</title>");
    sendFaviconLink(client);
    client.println("</head><body>");
    client.println("<h1><img src=\"/favicon.ico\" width=\"32\" height=\"32\"> Flicker Device Control</h1>");
    client.println("<p><a href=\"/config\">Configuration</a></p>");
    client.println("<form method=\"get\" action=\"/api/state\">");
    client.println("<p>Mode: <select name=\"mode\">");
    client.println("<option value=\"off\">Off</option>");
    client.println("<option value=\"constant\">Constant</option>");
    client.println("<option value=\"flicker\">Flicker</option>");
    client.println("<option value=\"sinus\">Sinus</option>");
    client.println("</select></p>");
    client.print("<p>Frequency (");
    client.print(MIN_FREQ_HZ);
    client.print("-");
    client.print(MAX_FREQ_HZ);
    client.print("): <input type=\"number\" name=\"freq\" min=\"");
    client.print(MIN_FREQ_HZ);
    client.print("\" max=\"");
    client.print(MAX_FREQ_HZ);
    client.print("\" value=\"");
    client.print(DEFAULT_FREQ_HZ);
    client.println("\"></p>");
    client.print("<p>Duty % (");
    client.print(MIN_DUTY_PERCENT);
    client.print("-");
    client.print(MAX_DUTY_PERCENT);
    client.print("): <input type=\"number\" name=\"duty\" min=\"");
    client.print(MIN_DUTY_PERCENT);
    client.print("\" max=\"");
    client.print(MAX_DUTY_PERCENT);
    client.print("\" value=\"");
    client.print(DEFAULT_DUTY_PERCENT);
    client.println("\"></p>");
    client.print("<p>Intensity % (");
    client.print(MIN_INTENSITY_PERCENT);
    client.print("-");
    client.print(MAX_INTENSITY_PERCENT);
    client.print("): <input type=\"number\" name=\"intensity\" min=\"");
    client.print(MIN_INTENSITY_PERCENT);
    client.print("\" max=\"");
    client.print(MAX_INTENSITY_PERCENT);
    client.print("\" value=\"");
    client.print(DEFAULT_INTENSITY_PERCENT);
    client.println("\"></p>");
    client.println("<p><button type=\"button\" onclick=\"apply()\">Apply</button></p>");

    client.println("</form>");
    client.println("<script>");
    client.println("function apply(){var m=document.querySelector('select[name=mode]').value; var f=document.querySelector('input[name=freq]').value; var d=document.querySelector('input[name=duty]').value; var i=document.querySelector('input[name=intensity]').value;");
    client.println("var x=new XMLHttpRequest(); x.open('POST','/api/'+m); x.setRequestHeader('Content-Type','application/json');");
    client.print("x.send(JSON.stringify({frequency:parseInt(f)||");
    client.print(DEFAULT_FREQ_HZ);
    client.print(",dutycycle:parseInt(d)||");
    client.print(DEFAULT_DUTY_PERCENT);
    client.print(",intensity:parseInt(i)||");
    client.print(DEFAULT_INTENSITY_PERCENT);
    client.println("}));}");
    client.println("</script>");
    client.println("</body></html>");
}

void WebServer::sendConfigPage(EthernetClient& client) {
    uint8_t ip[4], gw[4], sn[4];
    config_.getIp(ip);
    config_.getGateway(gw);
    config_.getSubnet(sn);
    char ipBuf[IPV4_STRING_BUFFER_LEN];
    char gwBuf[IPV4_STRING_BUFFER_LEN];
    char snBuf[IPV4_STRING_BUFFER_LEN];
    char currentIpBuf[IPV4_STRING_BUFFER_LEN];
    snprintf(ipBuf, sizeof(ipBuf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    snprintf(gwBuf, sizeof(gwBuf), "%u.%u.%u.%u", gw[0], gw[1], gw[2], gw[3]);
    snprintf(snBuf, sizeof(snBuf), "%u.%u.%u.%u", sn[0], sn[1], sn[2], sn[3]);
    formatIpv4(Ethernet.localIP(), currentIpBuf, sizeof(currentIpBuf));
    if (!currentIpBuf[0]) snprintf(currentIpBuf, sizeof(currentIpBuf), "--");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html><html><head><title>Configuration</title>");
    sendFaviconLink(client);
    client.println("</head><body>");
    client.println("<h1>Configuration</h1>");
    client.println("<form method=\"post\" action=\"/config\">");
    client.print("<p>PWM carrier Hz (");
    client.print(MIN_CARRIER_HZ);
    client.print("-");
    client.print(MAX_CARRIER_HZ);
    client.print("): <input type=\"number\" name=\"carrier\" min=\"");
    client.print(MIN_CARRIER_HZ);
    client.print("\" max=\"");
    client.print(MAX_CARRIER_HZ);
    client.print("\" value=\"");
    client.print(config_.getCarrierHz());
    client.println("\"></p>");
    if (config_.getUseDhcp()) {
        client.println("<p>DHCP: <input type=\"checkbox\" name=\"dhcp\" value=\"1\" checked></p>");
    } else {
        client.println("<p>DHCP: <input type=\"checkbox\" name=\"dhcp\" value=\"1\"></p>");
    }
    client.print("<p>IP: <input type=\"text\" name=\"ip\" value=\"");
    client.print(ipBuf);
    client.print("\" placeholder=\"192.168.1.100\"> (current: ");
    client.print(currentIpBuf);
    client.println(")</p>");
    client.print("<p>Gateway: <input type=\"text\" name=\"gw\" value=\"");
    client.print(gwBuf);
    client.print("\" placeholder=\"192.168.1.1\"> (current: ");
    client.print(gwBuf);
    client.println(")</p>");
    client.print("<p>Subnet: <input type=\"text\" name=\"sn\" value=\"");
    client.print(snBuf);
    client.print("\" placeholder=\"255.255.255.0\"> (current: ");
    client.print(snBuf);
    client.println(")</p>");
    client.println("<p><button type=\"submit\">Save</button></p>");
    client.println("</form>");
    client.println("<p><a href=\"/\">Back to control</a></p>");
    client.println("</body></html>");
}

static int parseIp(const char* s, uint8_t* out) {
    unsigned int a, b, c, d;
    if (sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
    out[0] = (uint8_t)a; out[1] = (uint8_t)b; out[2] = (uint8_t)c; out[3] = (uint8_t)d;
    return 1;
}

static const char* getFormValue(const char* body, const char* key, char* buf, size_t len) {
    size_t keyLen = strlen(key);
    const char* p = strstr(body, key);
    if (!p || p[keyLen] != '=') return nullptr;
    p += keyLen + 1;
    const char* end = strchr(p, '&');
    if (!end) end = p + strlen(p);
    size_t n = (size_t)(end - p);
    if (n >= len) n = len - 1;
    memcpy(buf, p, n);
    buf[n] = '\0';
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '+') buf[i] = ' ';
        else if (buf[i] == '%' && i + 2 < n) {
            unsigned int x;
            sscanf(buf + i + 1, "%2x", &x);
            buf[i] = (char)x;
            memmove(buf + i + 1, buf + i + 3, n - i - 2);
            n -= 2;
        }
    }
    buf[n] = '\0';
    return buf;
}

void WebServer::parseConfigPost(const char* body) {
    char buf[32];
    if (getFormValue(body, "carrier", buf, sizeof(buf))) {
        unsigned int hz = 0;
        if (sscanf(buf, "%u", &hz) == 1) {
            config_.setCarrierHz((uint32_t)hz);
            flicker_.setCarrierHz(config_.getCarrierHz());
        }
    }
    if (getFormValue(body, "dhcp", buf, sizeof(buf)) && (buf[0] == '1' || strcasecmp(buf, "on") == 0)) {
        config_.setUseDhcp(true);
    } else {
        config_.setUseDhcp(false);
        if (getFormValue(body, "ip", buf, sizeof(buf))) {
            uint8_t ip[4];
            if (parseIp(buf, ip)) config_.setIp(ip);
        }
        if (getFormValue(body, "gw", buf, sizeof(buf))) {
            uint8_t gw[4];
            if (parseIp(buf, gw)) config_.setGateway(gw);
        }
        if (getFormValue(body, "sn", buf, sizeof(buf))) {
            uint8_t sn[4];
            if (parseIp(buf, sn)) config_.setSubnet(sn);
        }
    }
}

void WebServer::sendJsonResponse(EthernetClient& client, int code, const char* body) {
    client.print("HTTP/1.1 ");
    client.print(code);
    if (code == 200) client.println(" OK");
    else if (code == 400) client.println(" Bad Request");
    else if (code == 404) client.println(" Not Found");
    else client.println(" Error");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(strlen(body));
    client.println();
    client.println(body);
}

void WebServer::handleApi(EthernetClient& client, const char* method, const char* path, const char* body) {
    if (strcmp(path, "identify") == 0 && strcmp(method, "GET") == 0) {
        char id[DEVICE_INFO_ID_HEX_BUFFER_LEN];
        DeviceInfo::writeDeviceIdHex(id, sizeof(id));
        char json[HTTP_API_IDENTIFY_JSON_CAP];
        snprintf(json, sizeof(json), "{\"device\":\"%s\",\"firmware\":\"%s\",\"id\":\"%s\"}",
                 DeviceInfo::deviceType(), DeviceInfo::firmwareVersion(), id);
        sendJsonResponse(client, 200, json);
        return;
    }
    if (strcmp(path, "state") == 0 && strcmp(method, "GET") == 0) {
        char json[128];
        snprintf(json, sizeof(json), "{\"mode\":\"%s\",\"frequency\":%lu,\"dutycycle\":%u,\"intensity\":%u}",
                 flicker_.getModeString(), (unsigned long)flicker_.getFrequencyHz(),
                 flicker_.getDutyPercent(), flicker_.getIntensityPercent());
        sendJsonResponse(client, 200, json);
        return;
    }
    if (strcmp(path, "off") == 0 && strcmp(method, "POST") == 0) {
        flicker_.setOff();
        sendJsonResponse(client, 200, "{\"ok\":true}");
        return;
    }
    if (strcmp(path, "constant") == 0 && strcmp(method, "POST") == 0) {
        parseAndApplyBody(flicker_, body);
        flicker_.setConstant(flicker_.getIntensityPercent());
        sendJsonResponse(client, 200, "{\"ok\":true}");
        return;
    }
    if (strcmp(path, "flicker") == 0 && strcmp(method, "POST") == 0) {
        parseAndApplyBody(flicker_, body);
        flicker_.setFlicker(flicker_.getFrequencyHz(), flicker_.getDutyPercent(), flicker_.getIntensityPercent());
        sendJsonResponse(client, 200, "{\"ok\":true}");
        return;
    }
    if (strcmp(path, "sinus") == 0 && strcmp(method, "POST") == 0) {
        parseAndApplyBody(flicker_, body);
        flicker_.setSinus(flicker_.getFrequencyHz(), flicker_.getIntensityPercent());
        sendJsonResponse(client, 200, "{\"ok\":true}");
        return;
    }
    if (strcmp(path, "frequency") == 0) {
        if (strcmp(method, "GET") == 0) {
            char json[48];
            snprintf(json, sizeof(json), "{\"frequency\":%lu}", (unsigned long)flicker_.getFrequencyHz());
            sendJsonResponse(client, 200, json);
            return;
        }
        if (strcmp(method, "POST") == 0) {
            unsigned int v = DEFAULT_FREQ_HZ;
            if (body) sscanf(body, "{\"value\":%u", &v);
            flicker_.setFrequency((uint32_t)v);
            sendJsonResponse(client, 200, "{\"ok\":true}");
            return;
        }
    }
    if (strcmp(path, "dutycycle") == 0) {
        if (strcmp(method, "GET") == 0) {
            char json[48];
            snprintf(json, sizeof(json), "{\"dutycycle\":%u}", flicker_.getDutyPercent());
            sendJsonResponse(client, 200, json);
            return;
        }
        if (strcmp(method, "POST") == 0) {
            unsigned int v = DEFAULT_DUTY_PERCENT;
            if (body) sscanf(body, "{\"value\":%u", &v);
            flicker_.setDutyCycle((uint8_t)v);
            sendJsonResponse(client, 200, "{\"ok\":true}");
            return;
        }
    }
    if (strcmp(path, "intensity") == 0) {
        if (strcmp(method, "GET") == 0) {
            char json[48];
            snprintf(json, sizeof(json), "{\"intensity\":%u}", flicker_.getIntensityPercent());
            sendJsonResponse(client, 200, json);
            return;
        }
        if (strcmp(method, "POST") == 0) {
            unsigned int v = DEFAULT_INTENSITY_PERCENT;
            if (body) sscanf(body, "{\"value\":%u", &v);
            flicker_.setIntensity((uint8_t)v);
            sendJsonResponse(client, 200, "{\"ok\":true}");
            return;
        }
    }
    sendJsonResponse(client, 400, "{\"error\":\"Bad request\"}");
}
