#include "htp1_client.h"
#include "config.h"
#include <WiFi.h>
#include <WebSocketClient.h>
#include <ArduinoJson.h>

static WiFiClient tcpClient;
static WebSocketClient wsClient;
static HTP1State state;
static char targetIP[40];
static uint16_t targetPort;
static unsigned long lastConnectAttempt = 0;
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

bool htp1_connect() {
    if (strlen(targetIP) == 0) return false;

    wsConnected = false;
    tcpClient.stop();

    Serial.printf("[HTP1] Connecting to %s:%d...\n", targetIP, targetPort);

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

    Serial.println("[HTP1] Connected");
    wsConnected = true;
    lastConnectAttempt = millis();
    return true;
}

// Parse a single JSON patch object
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
    else if (strcmp(path, "/input") == 0) {
        state.inputId = obj["value"].as<int>();
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

bool htp1_poll() {
    if (!wsConnected || !tcpClient.connected()) {
        wsConnected = false;

        // Throttled reconnect
        if (millis() - lastConnectAttempt < RECONNECT_INTERVAL_MS) {
            return false;
        }
        lastConnectAttempt = millis();
        Serial.println("[HTP1] Disconnected — attempting reconnect...");
        htp1_connect();
        return false;
    }

    String data;
    wsClient.getData(data);
    if (data.length() == 0) return false;

    // Strip "msoupdate " or "mso " prefix
    if (data.startsWith("msoupdate ")) {
        data = data.substring(10);
    } else if (data.startsWith("mso ")) {
        // Initial full state dump — very large, skip for now
        // TODO: parse select fields from full state if needed
        Serial.println("[HTP1] Received full state dump (skipped)");
        return false;
    }

    // Parse JSON array of patch objects
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data);
    if (err) {
        Serial.printf("[HTP1] JSON error: %s\n", err.c_str());
        return false;
    }

    bool anyUpdate = false;

    if (doc.is<JsonArray>()) {
        for (JsonObject obj : doc.as<JsonArray>()) {
            if (parse_patch(obj)) anyUpdate = true;
        }
    } else if (doc.is<JsonObject>()) {
        if (parse_patch(doc.as<JsonObject>())) anyUpdate = true;
    }

    if (anyUpdate) {
        state.changed = true;
    }

    return anyUpdate;
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
