#pragma once

#include <Arduino.h>
#include "settings.h"

// Forward declaration — avoids pulling full htp1_client.h into header
struct HTP1State;

// Initialize the display hardware (rm67162 + sprite)
void display_init();

// Render current HTP-1 state using the given theme & mode
void display_render(const HTP1State &state, const AppSettings &settings);

// Show a simple centered message (for splash / status / errors)
void display_show_message(const char* line1, const char* line2 = nullptr,
                          uint16_t color = 0xFFFF);

// Brightness control
void display_set_brightness(uint8_t raw);

// Display power
void display_on();
void display_off();
