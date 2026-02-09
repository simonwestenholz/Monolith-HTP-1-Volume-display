#include "htp1_client.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebSocketClient.h>
#include <ArduinoJson.h>

static WiFiClient tcpClient;
static WebSocketClient wsClient;
static HTP1State state;
static char targetIP[40];
static uint16_t targetPort;
static unsigned long lastConnectAttempt = 0;
static unsigned long lastHttpPoll = 0;
static bool wsConnected = false;

void htp1_init(const char* ip, uint16_t port, int8_t volumeOffset) {
    memset(&state, 0, sizeof(state));
    state.powerIsOn = true;
    state.volumeOffset = volumeOffset;
    strlcpy(targetIP, ip, sizeof(targetIP));
    targetPort = port;
}

void htp1_set_target(const char* ip, uint16_t port, int8_t volumeOffset) {
    strlcpy(targetIP, ip, sizeof(targetIP));
    targetPort = port;
    state.volumeOffset = volumeOffset;
}

// --- HTTP: fetch full state from /ircmd ---
static bool fetch_state_http() {
    if (strlen(targetIP) == 0) return false;

    HTTPClient http;
    String url = "http://";
    url += targetIP;
    url += "/ircmd";

    http.setTimeout(2000);
    http.begin(url);
    int code = http.GET();

    if (code != 200) {
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[HTP1] HTTP JSON error: %s\n", err.c_str());
        return false;
    }

    bool updated = false;

    if (doc["volume"].is<int>()) {
        int v = doc["volume"].as<int>();
        if (v != state.volume) { state.volume = v; updated = true; }
    }
    if (doc["muted"].is<bool>()) {
        bool m = doc["muted"].as<bool>();
        if (m != state.muted) { state.muted = m; updated = true; }
    }
    if (doc["input"].is<const char*>()) {
        const char* inp = doc["input"] | "";
        if (strcmp(inp, state.inputLabel) != 0) {
            strlcpy(state.inputLabel, inp, sizeof(state.inputLabel));
            updated = true;
        }
    }

    JsonObject status = doc["status"];
    if (status) {
        if (status["DECSourceProgram"].is<const char*>()) {
            const char* v = status["DECSourceProgram"] | "";
            if (strcmp(v, state.codecName) != 0) {
                strlcpy(state.codecName, v, sizeof(state.codecName));
                updated = true;
            }
        }
        if (status["DECProgramFormat"].is<const char*>()) {
            const char* v = status["DECProgramFormat"] | "";
            if (strcmp(v, state.programFormat) != 0) {
                strlcpy(state.programFormat, v, sizeof(state.programFormat));
                updated = true;
            }
        }
        if (status["SurroundMode"].is<const char*>()) {
            const char* v = status["SurroundMode"] | "";
            if (strcmp(v, state.surroundMode) != 0) {
                strlcpy(state.surroundMode, v, sizeof(state.surroundMode));
                updated = true;
            }
        }
        if (status["ENCListeningFormat"].is<const char*>()) {
            const char* v = status["ENCListeningFormat"] | "";
            if (strcmp(v, state.listeningFormat) != 0) {
                strlcpy(state.listeningFormat, v, sizeof(state.listeningFormat));
                updated = true;
            }
        }
    }

    if (updated) state.changed = true;
    return updated;
}

// --- WebSocket: parse a single JSON patch object ---
static bool parse_patch(JsonObject obj) {
    const char* path = obj["path"];
    if (!path) return false;

    bool updated = false;

    if (strcmp(path, "/volume") == 0) {
        state.volume = obj["value"].as<int>();
        updated = true;
    }
    else if (strcmp(path, "/muted") == 0) {
        state.muted = obj["value"].as<bool>();
        updated = true;
    }
    else if (strcmp(path, "/inputLabel") == 0) {
        strlcpy(state.inputLabel, obj["value"] | "", sizeof(state.inputLabel));
        updated = true;
    }
    else if (strcmp(path, "/status/DECSourceProgram") == 0) {
        strlcpy(state.codecName, obj["value"] | "", sizeof(state.codecName));
        updated = true;
    }
    else if (strcmp(path, "/status/DECProgramFormat") == 0) {
        strlcpy(state.programFormat, obj["value"] | "", sizeof(state.programFormat));
        updated = true;
    }
    else if (strcmp(path, "/status/SurroundMode") == 0) {
        strlcpy(state.surroundMode, obj["value"] | "", sizeof(state.surroundMode));
        updated = true;
    }
    else if (strcmp(path, "/status/ENCListeningFormat") == 0) {
        strlcpy(state.listeningFormat, obj["value"] | "", sizeof(state.listeningFormat));
        updated = true;
    }
    else if (strcmp(path, "/powerIsOn") == 0) {
        state.powerIsOn = obj["value"].as<bool>();
        updated = true;
    }

    return updated;
}

// --- WebSocket: connect ---
static bool ws_connect() {
    wsConnected = false;
    tcpClient.stop();

    Serial.printf("[HTP1] WebSocket connecting to %s:%d...\n", targetIP, targetPort);

    if (!tcpClient.connect(targetIP, targetPort)) {
        Serial.println("[HTP1] TCP connection failed");
        return false;
    }

    wsClient.path = (char*)HTP1_WS_PATH;
    wsClient.host = targetIP;

    if (!wsClient.handshake(tcpClient)) {
        Serial.println("[HTP1] WebSocket handshake failed");
        tcpClient.stop();
        return false;
    }

    Serial.println("[HTP1] WebSocket connected");
    wsConnected = true;
    lastConnectAttempt = millis();
    return true;
}

// --- WebSocket: poll for data ---
static bool ws_poll() {
    if (!wsConnected || !tcpClient.connected()) {
        wsConnected = false;

        // Throttled reconnect
        if (millis() - lastConnectAttempt < RECONNECT_INTERVAL_MS) {
            return false;
        }
        lastConnectAttempt = millis();
        ws_connect();
        return false;
    }

    String data;
    wsClient.getData(data);
    if (data.length() == 0) return false;

    // Strip prefix
    if (data.startsWith("msoupdate ")) {
        data = data.substring(10);
    } else if (data.startsWith("mso ")) {
        // Full state dump — skip, HTTP polling handles this
        return false;
    }

    // Parse JSON patch array
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data);
    if (err) return false;

    bool anyUpdate = false;

    if (doc.is<JsonArray>()) {
        for (JsonObject obj : doc.as<JsonArray>()) {
            if (parse_patch(obj)) anyUpdate = true;
        }
    } else if (doc.is<JsonObject>()) {
        if (parse_patch(doc.as<JsonObject>())) anyUpdate = true;
    }

    if (anyUpdate) state.changed = true;
    return anyUpdate;
}

// ============================================================
// Public API
// ============================================================

bool htp1_connect() {
    if (strlen(targetIP) == 0) return false;

    // Fetch full state via HTTP first
    Serial.printf("[HTP1] Fetching state from %s\n", targetIP);
    fetch_state_http();
    lastHttpPoll = millis();

    // Then open WebSocket for real-time updates
    return ws_connect();
}

bool htp1_poll() {
    if (strlen(targetIP) == 0) return false;

    // WebSocket: real-time volume/mute updates
    bool wsUpdate = ws_poll();

    // HTTP: periodic full state refresh (every 3s)
    bool httpUpdate = false;
    if (millis() - lastHttpPoll >= 3000) {
        lastHttpPoll = millis();
        httpUpdate = fetch_state_http();
    }

    return wsUpdate || httpUpdate;
}

const HTP1State& htp1_get_state() {
    return state;
}

void htp1_clear_changed() {
    state.changed = false;
}

bool htp1_connected() {
    return wsConnected && tcpClient.connected();
}
