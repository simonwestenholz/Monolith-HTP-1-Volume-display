// ============================================================
// HTP1_Display — Monolith HTP-1 Volume Display
// LilyGo T-Display-S3 AMOLED (536x240, ESP32-S3)
//
// Features: WebSocket volume display, web config UI, OTA,
//           4 display modes, 6 color themes, power management
// ============================================================

#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"
#include "settings.h"
#include "display_manager.h"
#include "htp1_client.h"
#include "button_handler.h"
#include "web_server.h"

// --- Global State ---
static AppSettings settings;
static bool ledOn = false;
static bool displayAsleep = false;
static bool apMode = false;

// Timing
static unsigned long lastActivityTime = 0;   // Last HTP-1 data or button press
static unsigned long nvsSavePending = 0;      // 0 = no pending save
static unsigned long lastRender = 0;

// --- Delayed NVS Save ---
static void schedule_save() {
    nvsSavePending = millis();
}

static void check_pending_save() {
    if (nvsSavePending == 0) return;
    if (millis() - nvsSavePending >= NVS_SAVE_DELAY_MS) {
        settings_save(settings);
        nvsSavePending = 0;
        Serial.println("[NVS] Settings saved");
    }
}

// --- Settings Changed Callback (from web server) ---
static void on_settings_changed() {
    // Apply display settings immediately
    display_set_brightness(BRIGHTNESS_PRESETS[settings.brightness_level]);

    // Update HTP-1 target
    htp1_set_target(settings.htp1_ip, settings.htp1_port, settings.volume_offset);

    lastActivityTime = millis();
}

// --- Wake display from sleep ---
static void wake_display() {
    if (displayAsleep) {
        display_on();
        displayAsleep = false;
    }
    display_set_brightness(BRIGHTNESS_PRESETS[settings.brightness_level]);
    lastActivityTime = millis();
}

// --- WiFi Connection ---
static bool connect_wifi() {
    if (strlen(settings.wifi_ssid) == 0) return false;

    display_show_message("Connecting WiFi...", settings.wifi_ssid);
    Serial.printf("[WIFI] Connecting to %s\n", settings.wifi_ssid);

    WiFi.setHostname(HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.wifi_ssid, settings.wifi_password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        if (millis() - start > WIFI_CONNECT_TIMEOUT) {
            Serial.println("[WIFI] Connection timeout");
            return false;
        }
    }

    Serial.printf("[WIFI] Connected: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

// --- AP Mode Fallback ---
static void start_ap_mode() {
    apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.printf("[WIFI] AP mode: %s @ %s\n", AP_SSID,
                  WiFi.softAPIP().toString().c_str());

    String msg = "http://";
    msg += WiFi.softAPIP().toString();
    display_show_message("Setup Mode", msg.c_str(), 0xFBE0);
}

// ============================================================
// setup()
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.printf("\n[HTP1] Firmware %s\n", FW_VERSION);

    // Load persistent settings
    settings_load(settings);

    // Initialize hardware
    display_init();
    buttons_init();

    // Splash screen
    display_show_message("HTP-1 Display", FW_VERSION);
    delay(1000);

    // WiFi
    if (!connect_wifi()) {
        start_ap_mode();
    }

    // mDNS
    if (!apMode) {
        if (MDNS.begin(HOSTNAME)) {
            Serial.printf("[mDNS] http://%s.local/\n", HOSTNAME);
        }
    }

    // Web server (works in both STA and AP mode)
    webserver_begin(&settings, on_settings_changed);

    // HTP-1 client
    htp1_init(settings.htp1_ip, settings.htp1_port, settings.volume_offset);

    if (!apMode && strlen(settings.htp1_ip) > 0) {
        display_show_message("Connecting HTP-1...", settings.htp1_ip);
        htp1_connect();
    }

    lastActivityTime = millis();

    // Show initial state or connection info
    if (apMode) {
        String msg = "http://";
        msg += WiFi.softAPIP().toString();
        display_show_message("Setup Mode", msg.c_str(), 0xFBE0);
    } else if (htp1_connected()) {
        display_show_message("Connected", WiFi.localIP().toString().c_str(), 0x07E0);
        delay(1000);
        // Render initial display
        display_render(htp1_get_state(), settings.color_theme, settings.display_mode);
    } else {
        String info = WiFi.localIP().toString();
        display_show_message("Waiting for HTP-1", info.c_str(), 0xFBE0);
    }
}

// ============================================================
// loop()
// ============================================================
void loop() {
    unsigned long now = millis();

    // --- Button Handling ---
    ButtonEvent btn = buttons_poll();
    if (btn != BTN_NONE) {
        wake_display();

        switch (btn) {
            case BTN1_SHORT:
                // Cycle brightness
                settings.brightness_level++;
                if (settings.brightness_level >= BRIGHTNESS_LEVELS)
                    settings.brightness_level = 0;
                display_set_brightness(BRIGHTNESS_PRESETS[settings.brightness_level]);
                schedule_save();
                break;

            case BTN1_LONG:
                // Cycle display mode
                settings.display_mode = (DisplayMode)((settings.display_mode + 1) % MODE_COUNT);
                display_render(htp1_get_state(), settings.color_theme, settings.display_mode);
                schedule_save();
                break;

            case BTN2_SHORT:
                // Toggle LED
                ledOn = !ledOn;
                digitalWrite(PIN_LED, ledOn);
                break;

            case BTN2_LONG:
                // Toggle sleep
                if (displayAsleep) {
                    wake_display();
                } else {
                    display_off();
                    displayAsleep = true;
                }
                break;

            default:
                break;
        }
    }

    // --- HTP-1 Polling ---
    bool newData = htp1_poll();
    if (newData) {
        wake_display();
        display_render(htp1_get_state(), settings.color_theme, settings.display_mode);
        htp1_clear_changed();
    }

    // --- Auto-dim ---
    if (!displayAsleep) {
        unsigned long elapsed = now - lastActivityTime;

        // Sleep (display off) after sleep timeout
        if (settings.sleep_enabled && elapsed > settings.sleep_timeout) {
            display_off();
            displayAsleep = true;
        }
        // Auto-dim after dim timeout
        else if (elapsed > settings.autodim_timeout) {
            display_set_brightness(settings.dim_brightness);
        }
    }

    // --- Periodic re-render (every 1s to catch web UI changes) ---
    if (!displayAsleep && (now - lastRender > 1000)) {
        lastRender = now;
        if (htp1_connected()) {
            display_render(htp1_get_state(), settings.color_theme, settings.display_mode);
        }
    }

    // --- Delayed NVS save ---
    check_pending_save();
}
