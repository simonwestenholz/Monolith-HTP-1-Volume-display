#include "button_handler.h"
#include "config.h"

#define DEBOUNCE_MS      50
#define LONG_PRESS_MS    1000

struct ButtonState {
    uint8_t pin;
    bool pressed;
    bool longFired;
    unsigned long pressTime;
    unsigned long lastChange;
};

static ButtonState btn1;  // GPIO 21 (PIN_BUTTON_2)
static ButtonState btn2;  // GPIO 0  (PIN_BUTTON_1)

void buttons_init() {
    pinMode(PIN_BUTTON_1, INPUT_PULLUP);
    pinMode(PIN_BUTTON_2, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);

    btn1 = { PIN_BUTTON_2, false, false, 0, 0 };
    btn2 = { PIN_BUTTON_1, false, false, 0, 0 };
}

static ButtonEvent poll_button(ButtonState &b, ButtonEvent shortEvt, ButtonEvent longEvt) {
    bool reading = (digitalRead(b.pin) == LOW);
    unsigned long now = millis();
    ButtonEvent evt = BTN_NONE;

    // Debounce
    if (reading != b.pressed && (now - b.lastChange) >= DEBOUNCE_MS) {
        b.lastChange = now;
        b.pressed = reading;

        if (b.pressed) {
            // Button just pressed
            b.pressTime = now;
            b.longFired = false;
        } else {
            // Button just released
            if (!b.longFired) {
                evt = shortEvt;
            }
        }
    }

    // Long press detection while held
    if (b.pressed && !b.longFired && (now - b.pressTime) >= LONG_PRESS_MS) {
        b.longFired = true;
        evt = longEvt;
    }

    return evt;
}

ButtonEvent buttons_poll() {
    ButtonEvent e;

    e = poll_button(btn1, BTN1_SHORT, BTN1_LONG);
    if (e != BTN_NONE) return e;

    e = poll_button(btn2, BTN2_SHORT, BTN2_LONG);
    if (e != BTN_NONE) return e;

    return BTN_NONE;
}
