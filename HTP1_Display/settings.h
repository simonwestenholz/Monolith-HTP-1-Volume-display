#pragma once

#include <Arduino.h>

// --- Color Theme ---
enum ColorTheme : uint8_t {
    THEME_WHITE = 0,
    THEME_GREEN,
    THEME_AMBER,
    THEME_BLUE,
    THEME_RED,
    THEME_CYAN,
    THEME_COUNT
};

// --- Display Mode ---
enum DisplayMode : uint8_t {
    MODE_VOLUME_ONLY = 0,
    MODE_VOLUME_SOURCE,
    MODE_VOLUME_CODEC,
    MODE_FULL_STATUS,
    MODE_COUNT
};

// --- Persistent Settings ---
struct AppSettings {
    // WiFi
    char wifi_ssid[64];
    char wifi_password[64];

    // HTP-1
    char htp1_ip[40];
    uint16_t htp1_port;
    int8_t volume_offset;

    // Display
    uint8_t brightness_level;   // Index into BRIGHTNESS_PRESETS
    uint32_t autodim_timeout;   // ms
    uint8_t dim_brightness;     // Raw brightness value when dimmed
    ColorTheme color_theme;
    DisplayMode display_mode;

    // Power
    bool sleep_enabled;
    uint32_t sleep_timeout;     // ms
};

// Load settings from NVS (fills defaults if no saved data)
void settings_load(AppSettings &s);

// Save current settings to NVS
void settings_save(const AppSettings &s);

// Reset settings to factory defaults and save
void settings_reset(AppSettings &s);
