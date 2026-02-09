#pragma once

// ============================================================
// HTP1_Display — Hardware & Application Configuration
// Merges pins_config.h + app-level defaults
// ============================================================

// --- Firmware ---
#define FW_VERSION        "1.0.1"
#define HOSTNAME          "htp1-display"

// --- Display Hardware (RM67162 AMOLED, 536x240) ---
#define LCD_USB_QSPI_DREVER  1

#define SPI_FREQUENCY         75000000
#define TFT_SPI_MODE          SPI_MODE0
#define TFT_SPI_HOST          SPI2_HOST

#define EXAMPLE_LCD_H_RES     536
#define EXAMPLE_LCD_V_RES     240
#define LVGL_LCD_BUF_SIZE     (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES)

#define TFT_WIDTH             240
#define TFT_HEIGHT            536
#define SEND_BUF_SIZE         (0x4000)

// Display pins (SPI)
#define TFT_TE                9
#define TFT_SDO               8
#define TFT_DC                7
#define TFT_RES               17
#define TFT_CS                6
#define TFT_MOSI              18
#define TFT_SCK               47

// Display pins (QSPI)
#define TFT_QSPI_CS           6
#define TFT_QSPI_SCK          47
#define TFT_QSPI_D0           18
#define TFT_QSPI_D1           7
#define TFT_QSPI_D2           48
#define TFT_QSPI_D3           5
#define TFT_QSPI_RST          17

// --- Peripheral Pins ---
#define PIN_LED               38
#define PIN_BAT_VOLT          4
#define PIN_BUTTON_1          0
#define PIN_BUTTON_2          21

// --- Brightness Presets ---
#define BRIGHTNESS_LEVELS     7
static const uint8_t BRIGHTNESS_PRESETS[BRIGHTNESS_LEVELS] = {
    7, 20, 25, 30, 50, 55, 60
};
#define BRIGHTNESS_DEFAULT    3      // Index into BRIGHTNESS_PRESETS

// --- Display Defaults ---
#define DISPLAY_WIDTH         536
#define DISPLAY_HEIGHT        240

// --- Timing Defaults ---
#define AUTODIM_TIMEOUT_MS    3000   // ms before auto-dim
#define DIM_BRIGHTNESS        7      // Brightness when dimmed
#define SLEEP_TIMEOUT_MS      60000  // ms before sleep (display off)
#define RECONNECT_INTERVAL_MS 5000   // ms between HTP-1 reconnect attempts
#define WIFI_CONNECT_TIMEOUT  30000  // ms to wait for WiFi before AP fallback
#define NVS_SAVE_DELAY_MS     5000   // Delayed NVS write to reduce flash wear

// --- HTP-1 Defaults ---
#define HTP1_DEFAULT_PORT     80
#define HTP1_WS_PATH          "/ws/controller"
#define HTP1_VOLUME_OFFSET    7      // Reference level offset

// --- WiFi AP Fallback ---
#define AP_SSID               "HTP1-Display-Setup"
#define AP_PASSWORD           ""     // Open network for initial setup
