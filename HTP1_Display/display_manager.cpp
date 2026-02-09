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
    // Secondary text color — visible but subdued
    switch (t) {
        case THEME_GREEN: return 0x03E0;  // Medium green
        case THEME_AMBER: return 0xC560;  // Medium amber
        case THEME_BLUE:  return 0x3BFF;  // Light blue
        case THEME_RED:   return 0xC000;  // Medium red
        case THEME_CYAN:  return 0x0577;  // Medium cyan
        default:          return 0xAD55;  // Light grey
    }
}

// --- Push sprite to hardware ---
static void push() {
    lcd_PushColors(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                   (uint16_t*)sprite.getPointer());
}

// --- Draw volume string ---
// textSize: 5=240px (full height), 3=144px, 2=96px with font 7
static void draw_volume(const HTP1State &state, uint16_t color, int y, uint8_t textSize) {
    sprite.setTextColor(color, TFT_BLACK);

    if (state.muted) {
        // Font 7 is 7-segment (digits only) — use font 4 for "MUTE"
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextSize(textSize > 3 ? 4 : textSize);
        sprite.drawString("MUTE", DISPLAY_WIDTH / 2, y, 4);
    } else {
        String vol = String(state.volume + state.volumeOffset);
        sprite.setTextDatum(TC_DATUM);
        sprite.setTextSize(textSize);
        sprite.drawString(vol, DISPLAY_WIDTH / 2, y, 7);
    }
    sprite.setTextDatum(TL_DATUM);  // Reset
}

// --- Build and abbreviate codec display string ---
static String build_codec_string(const char* codecName, const char* programFormat) {
    String s(codecName);
    if (strlen(programFormat) > 0) {
        s += "  ";
        s += programFormat;
    }
    // Abbreviate — order matters (longest/most specific first)
    s.replace("Object Audio", "");
    s.replace("(ATMOS)", "Atmos");
    s.replace("Dolby TrueHD", "TrueHD");
    s.replace("DTS-HD Master Audio", "DTS-HD MA");
    s.replace("DTS Legacy", "DTS");
    s.replace("DTS:X Object", "DTS:X");
    s.replace("Dolby Digital Plus", "DD+");
    s.replace("Dolby Digital", "DD");
    // Clean up double spaces from removals
    s.replace("  ", " ");
    s.trim();
    return s;
}

// --- Lookup friendly input name ---
static const char* lookup_input_name(const char* code, const AppSettings &settings) {
    for (uint8_t i = 0; i < settings.input_name_count && i < MAX_INPUT_NAMES; i++) {
        if (strcmp(code, settings.input_names[i].code) == 0 &&
            strlen(settings.input_names[i].name) > 0) {
            return settings.input_names[i].name;
        }
    }
    return code;  // Fall back to raw code
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

void display_render(const HTP1State &state, const AppSettings &settings) {
    ColorTheme theme = settings.color_theme;
    DisplayMode mode = settings.display_mode;
    uint8_t volSize   = settings.vol_sizes[mode];
    uint8_t labelSize = settings.label_sizes[mode];

    uint16_t fg = theme_color(theme);
    uint16_t dim = theme_dim(theme);
    uint16_t muteColor = 0xF800;  // Red for mute indication

    const char* inputDisplay = lookup_input_name(state.inputLabel, settings);

    static unsigned long lastDebugRender = 0;
    if (millis() - lastDebugRender > 5000) {
        lastDebugRender = millis();
        Serial.printf("[DISP] mode=%d theme=%d vol=%d vsz=%d lsz=%d input='%s'->'%s' codec='%s'\n",
                      mode, theme, state.volume + state.volumeOffset,
                      volSize, labelSize,
                      state.inputLabel, inputDisplay, state.codecName);
    }

    sprite.fillSprite(TFT_BLACK);

    uint16_t volColor = state.muted ? muteColor : fg;

    // Font 7 height per textSize unit: 48px
    // Font 4 height per textSize unit: ~26px
    int volH   = volSize * 48;
    int labelH = labelSize * 26;

    switch (mode) {
        case MODE_VOLUME_ONLY: {
            int y = (DISPLAY_HEIGHT - volH) / 2;
            draw_volume(state, volColor, y, volSize);
            break;
        }

        case MODE_VOLUME_SOURCE: {
            draw_label(inputDisplay, dim, 10, 0, 4, labelSize);
            int y = labelH + 4;
            draw_volume(state, volColor, y, volSize);
            break;
        }

        case MODE_VOLUME_CODEC: {
            draw_volume(state, volColor, 0, volSize);
            // Codec / format anchored at bottom
            String codecLine = build_codec_string(state.codecName, state.programFormat);
            sprite.setTextDatum(BC_DATUM);
            // Auto-shrink if too wide
            sprite.setTextSize(labelSize);
            int cw = sprite.textWidth(codecLine, 4);
            uint8_t csz = (cw > DISPLAY_WIDTH - 20 && labelSize > 1) ? labelSize - 1 : labelSize;
            draw_label(codecLine.c_str(), dim, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 2, 4, csz);
            sprite.setTextDatum(TL_DATUM);
            break;
        }

        case MODE_FULL_STATUS: {
            // Top row: source left, codec right
            draw_label(inputDisplay, dim, 10, 0, 4, labelSize);
            {
                String codec = build_codec_string(state.codecName, state.programFormat);
                sprite.setTextSize(labelSize);
                int codecWidth = sprite.textWidth(codec, 4);
                // Auto-shrink if codec overlaps with input label
                uint8_t csz = (codecWidth > DISPLAY_WIDTH / 2 && labelSize > 1) ? labelSize - 1 : labelSize;
                if (csz < labelSize) {
                    sprite.setTextSize(csz);
                    codecWidth = sprite.textWidth(codec, 4);
                }
                draw_label(codec.c_str(), dim, DISPLAY_WIDTH - codecWidth - 10, 0, 4, csz);
            }

            // Volume: centered between top and bottom labels
            int topY = labelH + 2;
            int botLabelY = DISPLAY_HEIGHT - labelH;
            int volY = topY + (botLabelY - topY - volH) / 2;
            if (volY < topY) volY = topY;
            draw_volume(state, volColor, volY, volSize);

            // Bottom row: surround left, listening format right
            draw_label(state.surroundMode, dim, 10, botLabelY, 4, labelSize);
            {
                sprite.setTextSize(labelSize);
                int lfWidth = sprite.textWidth(state.listeningFormat, 4);
                draw_label(state.listeningFormat, dim,
                           DISPLAY_WIDTH - lfWidth - 10, botLabelY, 4, labelSize);
            }
            break;
        }

        default: {
            int y = (DISPLAY_HEIGHT - volH) / 2;
            draw_volume(state, volColor, y, volSize);
            break;
        }
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
