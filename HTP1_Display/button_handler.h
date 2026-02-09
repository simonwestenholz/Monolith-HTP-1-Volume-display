#pragma once

#include <Arduino.h>

// Button event types
enum ButtonEvent : uint8_t {
    BTN_NONE = 0,
    BTN1_SHORT,   // GPIO 21 short press — cycle brightness
    BTN1_LONG,    // GPIO 21 long press  — cycle display mode
    BTN2_SHORT,   // GPIO 0  short press — toggle LED
    BTN2_LONG,    // GPIO 0  long press  — toggle sleep
};

// Initialize button GPIOs
void buttons_init();

// Poll buttons — call from loop(). Returns event if one fired.
ButtonEvent buttons_poll();
