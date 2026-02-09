#include "web_server.h"
#include "config.h"
#include "web_ui.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>

static AsyncWebServer server(80);
static AppSettings *cfg = nullptr;
static void (*settingsChangedCb)() = nullptr;

// --- GET / — serve the web UI ---
static void handleRoot(AsyncWebServerRequest *req) {
    req->send_P(200, "text/html", WEB_HTML);
}

// --- GET /status — live status JSON ---
static void handleStatus(AsyncWebServerRequest *req) {
    const HTP1State &st = htp1_get_state();
    JsonDocument doc;

    doc["wifi"]  = (WiFi.status() == WL_CONNECTED);
    doc["ip"]    = WiFi.localIP().toString();
    doc["rssi"]  = WiFi.RSSI();
    doc["htp1"]  = htp1_connected();
    doc["vol"]   = st.volume + st.volumeOffset;
    doc["muted"] = st.muted;
    doc["input"] = st.inputLabel;
    doc["codec"] = st.codecName;
    doc["power"] = st.powerIsOn;

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
}

// --- GET /settings — current settings as JSON ---
static void handleGetSettings(AsyncWebServerRequest *req) {
    if (!cfg) { req->send(500); return; }
    JsonDocument doc;

    doc["ssid"]     = cfg->wifi_ssid;
    doc["pass"]     = "";  // Never send password back
    doc["htp1ip"]   = cfg->htp1_ip;
    doc["htp1port"] = cfg->htp1_port;
    doc["voloff"]   = cfg->volume_offset;
    doc["bright"]   = cfg->brightness_level;
    doc["dimtime"]  = cfg->autodim_timeout;
    doc["dimbrt"]   = cfg->dim_brightness;
    doc["dmode"]    = (uint8_t)cfg->display_mode;
    doc["theme"]    = (uint8_t)cfg->color_theme;
    doc["sleepen"]  = cfg->sleep_enabled;
    doc["sleeptm"]  = cfg->sleep_timeout;
    doc["fw"]       = FW_VERSION;

    String json;
    serializeJson(doc, json);
    req->send(200, "application/json", json);
}

// --- POST /settings — update settings ---
static void handlePostSettings(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!cfg) { req->send(500); return; }

    // Accumulate body (may arrive in chunks)
    static String body;
    if (index == 0) body = "";
    body += String((char*)data).substring(0, len);
    if (index + len < total) return;  // Wait for more chunks

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    body = "";

    if (err) {
        req->send(400, "application/json", "{\"ok\":false,\"error\":\"bad json\"}");
        return;
    }

    // Factory reset
    if (doc["reset"].as<bool>()) {
        settings_reset(*cfg);
        if (settingsChangedCb) settingsChangedCb();
        req->send(200, "application/json", "{\"ok\":true}");
        return;
    }

    // Apply fields
    if (doc["ssid"].is<const char*>())
        strlcpy(cfg->wifi_ssid, doc["ssid"] | "", sizeof(cfg->wifi_ssid));
    if (doc["pass"].is<const char*>()) {
        const char* p = doc["pass"] | "";
        if (strlen(p) > 0)  // Only update if non-empty
            strlcpy(cfg->wifi_password, p, sizeof(cfg->wifi_password));
    }
    if (doc["htp1ip"].is<const char*>())
        strlcpy(cfg->htp1_ip, doc["htp1ip"] | "", sizeof(cfg->htp1_ip));
    if (doc["htp1port"].is<int>())
        cfg->htp1_port = doc["htp1port"];
    if (doc["voloff"].is<int>())
        cfg->volume_offset = doc["voloff"];
    if (doc["bright"].is<int>())
        cfg->brightness_level = constrain(doc["bright"].as<int>(), 0, BRIGHTNESS_LEVELS - 1);
    if (doc["dimtime"].is<int>())
        cfg->autodim_timeout = doc["dimtime"];
    if (doc["dimbrt"].is<int>())
        cfg->dim_brightness = constrain(doc["dimbrt"].as<int>(), 1, 255);
    if (doc["dmode"].is<int>())
        cfg->display_mode = (DisplayMode)constrain(doc["dmode"].as<int>(), 0, MODE_COUNT - 1);
    if (doc["theme"].is<int>())
        cfg->color_theme = (ColorTheme)constrain(doc["theme"].as<int>(), 0, THEME_COUNT - 1);
    if (doc["sleepen"].is<bool>())
        cfg->sleep_enabled = doc["sleepen"];
    if (doc["sleeptm"].is<int>())
        cfg->sleep_timeout = doc["sleeptm"];

    settings_save(*cfg);
    if (settingsChangedCb) settingsChangedCb();

    req->send(200, "application/json", "{\"ok\":true}");
}

// --- POST /update — OTA firmware upload ---
static void handleOTAUpload(AsyncWebServerRequest *req, const String& filename,
                             size_t index, uint8_t *data, size_t len, bool final) {
    if (index == 0) {
        Serial.printf("[OTA] Begin: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    }

    if (Update.isRunning()) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
    }

    if (final) {
        if (Update.end(true)) {
            Serial.printf("[OTA] Success: %u bytes\n", index + len);
        } else {
            Update.printError(Serial);
        }
    }
}

static void handleOTADone(AsyncWebServerRequest *req) {
    bool ok = !Update.hasError();
    req->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
    if (ok) {
        delay(500);
        ESP.restart();
    }
}

// ============================================================
// Public
// ============================================================

void webserver_begin(AppSettings *settings, void (*onSettingsChanged)()) {
    cfg = settings;
    settingsChangedCb = onSettingsChanged;

    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/settings", HTTP_GET, handleGetSettings);

    // POST /settings with JSON body
    server.onRequestBody([](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
        if (req->url() == "/settings" && req->method() == HTTP_POST) {
            handlePostSettings(req, data, len, index, total);
        }
    });

    server.on("/update", HTTP_POST, handleOTADone, handleOTAUpload);

    server.begin();
    Serial.println("[WEB] Server started on port 80");
}
