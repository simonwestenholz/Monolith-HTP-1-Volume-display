#include "settings.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;
static const char* NS = "htp1disp";

static void apply_defaults(AppSettings &s) {
    strlcpy(s.wifi_ssid,     "",              sizeof(s.wifi_ssid));
    strlcpy(s.wifi_password,  "",              sizeof(s.wifi_password));
    strlcpy(s.htp1_ip,       "",              sizeof(s.htp1_ip));
    s.htp1_port        = HTP1_DEFAULT_PORT;
    s.volume_offset    = HTP1_VOLUME_OFFSET;
    s.brightness_level = BRIGHTNESS_DEFAULT;
    s.autodim_timeout  = AUTODIM_TIMEOUT_MS;
    s.dim_brightness   = DIM_BRIGHTNESS;
    s.color_theme      = THEME_WHITE;
    s.display_mode     = MODE_VOLUME_ONLY;
    s.sleep_enabled    = false;
    s.sleep_timeout    = SLEEP_TIMEOUT_MS;
}

void settings_load(AppSettings &s) {
    apply_defaults(s);
    prefs.begin(NS, true);  // read-only

    if (prefs.isKey("ssid")) {
        strlcpy(s.wifi_ssid,    prefs.getString("ssid", "").c_str(),   sizeof(s.wifi_ssid));
        strlcpy(s.wifi_password, prefs.getString("pass", "").c_str(),   sizeof(s.wifi_password));
        strlcpy(s.htp1_ip,     prefs.getString("htp1ip", "").c_str(), sizeof(s.htp1_ip));
        s.htp1_port        = prefs.getUShort("htp1port", HTP1_DEFAULT_PORT);
        s.volume_offset    = prefs.getChar("voloff",     HTP1_VOLUME_OFFSET);
        s.brightness_level = prefs.getUChar("bright",    BRIGHTNESS_DEFAULT);
        s.autodim_timeout  = prefs.getULong("dimtime",   AUTODIM_TIMEOUT_MS);
        s.dim_brightness   = prefs.getUChar("dimbrt",    DIM_BRIGHTNESS);
        s.color_theme      = (ColorTheme)prefs.getUChar("theme",  THEME_WHITE);
        s.display_mode     = (DisplayMode)prefs.getUChar("dmode",  MODE_VOLUME_ONLY);
        s.sleep_enabled    = prefs.getBool("sleepen",    false);
        s.sleep_timeout    = prefs.getULong("sleeptm",   SLEEP_TIMEOUT_MS);
    }

    prefs.end();
}

void settings_save(const AppSettings &s) {
    prefs.begin(NS, false);  // read-write

    prefs.putString("ssid",     s.wifi_ssid);
    prefs.putString("pass",     s.wifi_password);
    prefs.putString("htp1ip",   s.htp1_ip);
    prefs.putUShort("htp1port", s.htp1_port);
    prefs.putChar("voloff",     s.volume_offset);
    prefs.putUChar("bright",    s.brightness_level);
    prefs.putULong("dimtime",   s.autodim_timeout);
    prefs.putUChar("dimbrt",    s.dim_brightness);
    prefs.putUChar("theme",     (uint8_t)s.color_theme);
    prefs.putUChar("dmode",     (uint8_t)s.display_mode);
    prefs.putBool("sleepen",    s.sleep_enabled);
    prefs.putULong("sleeptm",   s.sleep_timeout);

    prefs.end();
}

void settings_reset(AppSettings &s) {
    prefs.begin(NS, false);
    prefs.clear();
    prefs.end();
    apply_defaults(s);
    settings_save(s);
}
