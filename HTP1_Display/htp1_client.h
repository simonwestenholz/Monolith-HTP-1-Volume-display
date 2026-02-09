#pragma once

#include <Arduino.h>

// --- HTP-1 State ---
struct HTP1State {
    int   volume;
    int8_t volumeOffset;
    bool  muted;
    int   inputId;
    char  inputLabel[32];
    char  codecName[32];
    char  programFormat[32];
    char  surroundMode[32];
    char  listeningFormat[32];
    bool  powerIsOn;

    // Change flags — set true when a field updates, cleared by consumer
    bool changed;
};

// Initialize HTP-1 client (call once in setup)
void htp1_init(const char* ip, uint16_t port, int8_t volumeOffset);

// Update connection target (e.g. after settings change)
void htp1_set_target(const char* ip, uint16_t port, int8_t volumeOffset);

// Attempt connection / reconnection to HTP-1
// Returns true if connected
bool htp1_connect();

// Poll for new data. Call from loop().
// Returns true if new data was received & parsed.
bool htp1_poll();

// Get current state (read-only reference)
const HTP1State& htp1_get_state();

// Clear the change flag
void htp1_clear_changed();

// Is the WebSocket currently connected?
bool htp1_connected();
