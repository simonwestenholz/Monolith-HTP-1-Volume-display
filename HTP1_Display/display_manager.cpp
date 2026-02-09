#include "display_manager.h"
#include "config.h"
#include "htp1_client.h"
#include "rm67162.h"
#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();
static TFT_eSprite sprite = TFT_eSprite(&tft);

// --- Theme Color Table (RGB565) ---
static uint16_t theme_color(ColorTheme t) {
    switch (t) {
        case THEME_GREEN: return 0x07E0;  // TFT_GREEN
        case THEME_AMBER: return 0xFBE0;  // Amber (~255,190,0)
        case THEME_BLUE:  return 0x001F;  // TFT_BLUE — use a brighter blue
        case THEME_RED:   return 0xF800;  // TFT_RED
        case THEME_CYAN:  return 0x07FF;  // TFT_CYAN
        default:          return 0xFFFF;  // TFT_WHITE
    }
}

static uint16_t theme_dim(ColorTheme t) {
    // Dimmer variant for secondary text
    switch (t) {
        case THEME_GREEN: return 0x0400;
        case THEME_AMBER: return 0x7980;
        case THEME_BLUE:  return 0x0010;
        case THEME_RED:   return 0x7800;
        case THEME_CYAN:  return 0x0410;
        default:          return 0x7BEF;  // Grey
    }
}

// --- Push sprite to hardware ---
static void push() {
    lcd_PushColors(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                   (uint16_t*)sprite.getPointer());
}

// --- Draw volume string, centered ---
static void draw_volume_centered(const HTP1State &state, uint16_t color, int y) {
    sprite.setTextColor(color, TFT_BLACK);

    if (state.muted) {
        sprite.setTextSize(4);
        sprite.setTextDatum(TC_DATUM);
        sprite.drawString("MUTE", DISPLAY_WIDTH / 2, y, 7);
    } else {
        String vol = String(state.volume + state.volumeOffset);
        sprite.setTextSize(5);
        sprite.setTextDatum(TC_DATUM);
        sprite.drawString(vol, DISPLAY_WIDTH / 2, y, 7);
    }
    sprite.setTextDatum(TL_DATUM);  // Reset
}

// --- Draw secondary info line ---
static void draw_label(const char* text, uint16_t color, int x, int y, uint8_t font, uint8_t size) {
    sprite.setTextColor(color, TFT_BLACK);
    sprite.setTextSize(size);
    sprite.drawString(text, x, y, font);
}

// ============================================================
// Public API
// ============================================================

void display_init() {
    rm67162_init();
    lcd_setRotation(1);
    sprite.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    sprite.setSwapBytes(1);
    lcd_brightness(BRIGHTNESS_PRESETS[BRIGHTNESS_DEFAULT]);
}

void display_render(const HTP1State &state, ColorTheme theme, DisplayMode mode) {
    uint16_t fg = theme_color(theme);
    uint16_t dim = theme_dim(theme);
    uint16_t muteColor = 0xF800;  // Red for mute indication

    sprite.fillSprite(TFT_BLACK);

    uint16_t volColor = state.muted ? muteColor : fg;

    switch (mode) {
        case MODE_VOLUME_ONLY:
            draw_volume_centered(state, volColor, 50);
            break;

        case MODE_VOLUME_SOURCE:
            draw_label(state.inputLabel, dim, 10, 8, 4, 1);
            draw_volume_centered(state, volColor, 60);
            break;

        case MODE_VOLUME_CODEC: {
            draw_volume_centered(state, volColor, 30);
            // Codec / format at bottom
            String codecLine = String(state.codecName);
            if (strlen(state.programFormat) > 0) {
                codecLine += "  ";
                codecLine += state.programFormat;
            }
            sprite.setTextDatum(BC_DATUM);
            draw_label(codecLine.c_str(), dim, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 10, 4, 1);
            sprite.setTextDatum(TL_DATUM);
            break;
        }

        case MODE_FULL_STATUS: {
            // Top: source + codec
            draw_label(state.inputLabel, dim, 10, 5, 4, 1);
            {
                String codec = String(state.codecName);
                if (strlen(state.programFormat) > 0) {
                    codec += "  ";
                    codec += state.programFormat;
                }
                int codecWidth = sprite.textWidth(codec, 4);
                draw_label(codec.c_str(), dim, DISPLAY_WIDTH - codecWidth - 10, 5, 4, 1);
            }

            // Center: volume
            draw_volume_centered(state, volColor, 55);

            // Bottom: surround mode + listening format
            draw_label(state.surroundMode, dim, 10, DISPLAY_HEIGHT - 28, 4, 1);
            {
                int lfWidth = sprite.textWidth(state.listeningFormat, 4);
                draw_label(state.listeningFormat, dim,
                           DISPLAY_WIDTH - lfWidth - 10, DISPLAY_HEIGHT - 28, 4, 1);
            }
            break;
        }

        default:
            draw_volume_centered(state, volColor, 50);
            break;
    }

    // Power-off indicator
    if (!state.powerIsOn) {
        sprite.setTextDatum(BC_DATUM);
        draw_label("STANDBY", muteColor, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 5, 4, 1);
        sprite.setTextDatum(TL_DATUM);
    }

    push();
}

void display_show_message(const char* line1, const char* line2, uint16_t color) {
    sprite.fillSprite(TFT_BLACK);
    sprite.setTextColor(color, TFT_BLACK);
    sprite.setTextDatum(MC_DATUM);

    if (line2) {
        sprite.setTextSize(2);
        sprite.drawString(line1, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 30, 4);
        sprite.setTextSize(1);
        sprite.drawString(line2, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 + 30, 4);
    } else {
        sprite.setTextSize(2);
        sprite.drawString(line1, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 4);
    }

    sprite.setTextDatum(TL_DATUM);
    push();
}

void display_set_brightness(uint8_t raw) {
    lcd_brightness(raw);
}

void display_on() {
    lcd_display_on();
}

void display_off() {
    lcd_display_off();
}
