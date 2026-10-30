#include "core/input.h"
#include "core/util.h"
#include "core/math.h"
#include <cmath>

namespace sv {

void Input::init() {
    m_since.fill(1e9f);
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    if (ids && n > 0) m_pad = SDL_OpenGamepad(ids[0]);
    SDL_free(ids);
    detectPad();
}

void Input::detectPad() {
    if (!m_pad) return;
    SDL_GamepadType t = SDL_GetGamepadType(m_pad);
    m_padStyle = (t == SDL_GAMEPAD_TYPE_PS3 || t == SDL_GAMEPAD_TYPE_PS4 || t == SDL_GAMEPAD_TYPE_PS5) ? Style::PlayStation
               : (t == SDL_GAMEPAD_TYPE_XBOX360 || t == SDL_GAMEPAD_TYPE_XBOXONE) ? Style::Xbox : Style::PlayStation;
    m_hasLed = SDL_GetBooleanProperty(SDL_GetGamepadProperties(m_pad), SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN, false);
    m_led[0] = -1;
    m_gamepadActive = true;
    logInfo("Gamepad: %s (%s layout%s)", SDL_GetGamepadName(m_pad), m_padStyle == Style::Xbox ? "Xbox" : "PlayStation", m_hasLed ? ", light bar" : "");
}

void Input::shutdown() {
    if (m_pad) SDL_CloseGamepad(m_pad);
    m_pad = nullptr;
}

void Input::handleEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!m_pad) { m_pad = SDL_OpenGamepad(e.gdevice.which); detectPad(); }
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (m_pad && SDL_GetGamepadID(m_pad) == e.gdevice.which) { SDL_CloseGamepad(m_pad); m_pad = nullptr; m_gamepadActive = false; m_hasLed = false; }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN: case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || std::abs(e.gaxis.value) > 16000) m_gamepadActive = true;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        break;
    case SDL_EVENT_KEY_DOWN: m_gamepadActive = false; break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        m_gamepadActive = false;
        if (e.button.button == SDL_BUTTON_LEFT) m_mouseL = true;
        if (e.button.button == SDL_BUTTON_RIGHT) m_mouseR = true;
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e.button.button == SDL_BUTTON_LEFT) m_mouseL = false;
        if (e.button.button == SDL_BUTTON_RIGHT) m_mouseR = false;
        break;
    default: break;
    }
}

InputFrame Input::poll() {
    InputFrame f;
    const bool* k = SDL_GetKeyboardState(nullptr);
    auto set = [&](Act a, bool v) { if (v) f.down[(size_t)a] = true; };
    set(Act::Left, k[SDL_SCANCODE_A] || k[SDL_SCANCODE_LEFT]);
    set(Act::Right, k[SDL_SCANCODE_D] || k[SDL_SCANCODE_RIGHT]);
    set(Act::Up, k[SDL_SCANCODE_W] || k[SDL_SCANCODE_UP]);
    set(Act::Down, k[SDL_SCANCODE_S] || k[SDL_SCANCODE_DOWN]);
    set(Act::Jump, k[SDL_SCANCODE_SPACE]);
    set(Act::Attack, k[SDL_SCANCODE_J] || m_mouseL);
    set(Act::Guard, k[SDL_SCANCODE_K] || m_mouseR);
    set(Act::Dash, k[SDL_SCANCODE_L] || k[SDL_SCANCODE_LSHIFT]);
    set(Act::Grapple, k[SDL_SCANCODE_I] || k[SDL_SCANCODE_F]);
    set(Act::Shuriken, k[SDL_SCANCODE_U] || k[SDL_SCANCODE_R]);
    set(Act::Art, k[SDL_SCANCODE_O] || k[SDL_SCANCODE_Q]);
    set(Act::Heal, k[SDL_SCANCODE_H] || k[SDL_SCANCODE_C]);
    set(Act::Interact, k[SDL_SCANCODE_W] || k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_E]);
    set(Act::Pause, k[SDL_SCANCODE_ESCAPE]);
    set(Act::Menu, k[SDL_SCANCODE_TAB]);
    set(Act::Confirm, k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_J] || k[SDL_SCANCODE_KP_ENTER]);
    set(Act::Cancel, k[SDL_SCANCODE_ESCAPE] || k[SDL_SCANCODE_BACKSPACE] || k[SDL_SCANCODE_K]);
    set(Act::TabPrev, k[SDL_SCANCODE_Q] || k[SDL_SCANCODE_PAGEUP]);
    set(Act::TabNext, k[SDL_SCANCODE_E] || k[SDL_SCANCODE_PAGEDOWN]);
    f.moveX = (f.held(Act::Right) ? 1.f : 0.f) - (f.held(Act::Left) ? 1.f : 0.f);
    f.moveY = (f.held(Act::Down) ? 1.f : 0.f) - (f.held(Act::Up) ? 1.f : 0.f);

    if (m_pad) {
        auto btn = [&](SDL_GamepadButton b) { return SDL_GetGamepadButton(m_pad, b); };
        auto axis = [&](SDL_GamepadAxis a) { return SDL_GetGamepadAxis(m_pad, a) / 32767.f; };
        // radial deadzone + response curve: small tilts give fine, quiet movement
        auto stick = [&](SDL_GamepadAxis ax, SDL_GamepadAxis ay, float dz, float curve, float& ox, float& oy) {
            float x = axis(ax), y = axis(ay), m = std::sqrt(x * x + y * y);
            if (m < dz) { ox = oy = 0; return; }
            float k = std::pow(std::min(1.f, (m - dz) / (1 - dz)), curve) / m;
            ox = x * k; oy = y * k;
        };
        float lx, ly, rx, ry;
        stick(SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY, 0.18f, 1.4f, lx, ly);
        stick(SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY, 0.22f, 1.0f, rx, ry);
        if (btn(SDL_GAMEPAD_BUTTON_DPAD_LEFT)) lx = -1;
        if (btn(SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) lx = 1;
        if (btn(SDL_GAMEPAD_BUTTON_DPAD_UP)) ly = -1;
        if (btn(SDL_GAMEPAD_BUTTON_DPAD_DOWN)) ly = 1;
        if (lx != 0 || ly != 0) { f.moveX = clampf(f.moveX + lx, -1, 1); f.moveY = clampf(f.moveY + ly, -1, 1); }
        f.aimX = rx; f.aimY = ry;
        set(Act::Left, lx < -0.5f); set(Act::Right, lx > 0.5f);
        set(Act::Up, ly < -0.65f); set(Act::Down, ly > 0.65f);
        set(Act::Interact, btn(SDL_GAMEPAD_BUTTON_DPAD_UP) || btn(SDL_GAMEPAD_BUTTON_NORTH));
        // Duel layout: thumbs jump & step, index fingers strike-timing on the shoulders
        set(Act::Jump, btn(SDL_GAMEPAD_BUTTON_SOUTH));                 // ✕ / A
        set(Act::Dash, btn(SDL_GAMEPAD_BUTTON_EAST));                  // ○ / B
        set(Act::Attack, btn(SDL_GAMEPAD_BUTTON_WEST));                // □ / X
        set(Act::Art, btn(SDL_GAMEPAD_BUTTON_NORTH));                  // △ / Y (Interact when prompted)
        set(Act::Guard, btn(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));        // L1
        set(Act::Grapple, btn(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));     // R1
        set(Act::Heal, axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 0.4f);    // L2
        set(Act::Shuriken, axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.4f); // R2
        set(Act::Pause, btn(SDL_GAMEPAD_BUTTON_START));                // OPTIONS
        set(Act::Menu, btn(SDL_GAMEPAD_BUTTON_BACK) || btn(SDL_GAMEPAD_BUTTON_TOUCHPAD));
        set(Act::Confirm, btn(SDL_GAMEPAD_BUTTON_SOUTH));
        set(Act::Cancel, btn(SDL_GAMEPAD_BUTTON_EAST));
        set(Act::TabPrev, btn(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
        set(Act::TabNext, btn(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
    }
    return f;
}

void Input::tick(float dt) {
    m_prev = m_cur;
    m_cur = m_injected ? m_injectedFrame : poll();
    if (guardToggle && !m_injected) {
        bool raw = m_cur.held(Act::Guard);
        if (raw && !m_rawGuardPrev) m_guardToggled = !m_guardToggled;
        m_rawGuardPrev = raw;
        m_cur.down[(size_t)Act::Guard] = m_guardToggled;
    }
    for (size_t i = 0; i < (size_t)Act::Count; i++) {
        if (m_cur.down[i] && !m_prev.down[i]) m_since[i] = 0;
        else if (m_since[i] < 1e8f) m_since[i] += dt;
    }
}

void Input::injectFrame(const InputFrame& f) { m_injectedFrame = f; m_injected = true; }

std::string Input::keyLabel(Act a) const {
    switch (a) {
    case Act::Jump: return "Space";
    case Act::Attack: return "J";
    case Act::Guard: return "K";
    case Act::Dash: return "Shift";
    case Act::Grapple: return "F";
    case Act::Shuriken: return "R";
    case Act::Art: return "Q";
    case Act::Heal: return "C";
    case Act::Interact: return "E";
    case Act::Up: return "W";
    case Act::Down: return "S";
    case Act::Left: return "A";
    case Act::Right: return "D";
    case Act::Pause: return "Esc";
    case Act::Menu: return "Tab";
    case Act::Confirm: return "Enter";
    case Act::Cancel: return "Esc";
    case Act::TabPrev: return "PgUp";
    case Act::TabNext: return "PgDn";
    default: return "?";
    }
}

std::string Input::glyph(Act a) const {
    if (style() == Style::Keyboard) return keyLabel(a);
    bool ps = m_padStyle == Style::PlayStation;
    switch (a) {
    case Act::Jump: case Act::Confirm: return ps ? "Cross" : "A";
    case Act::Dash: case Act::Cancel: return ps ? "Circle" : "B";
    case Act::Attack: return ps ? "Square" : "X";
    case Act::Art: case Act::Interact: return ps ? "Triangle" : "Y";
    case Act::Guard: case Act::TabPrev: return ps ? "L1" : "LB";
    case Act::Grapple: case Act::TabNext: return ps ? "R1" : "RB";
    case Act::Heal: return ps ? "L2" : "LT";
    case Act::Shuriken: return ps ? "R2" : "RT";
    case Act::Pause: return ps ? "OPTIONS" : "MENU";
    case Act::Menu: return ps ? "TOUCHPAD" : "VIEW";
    case Act::Up: return "Up";
    case Act::Down: return "Down";
    default: return "?";
    }
}

void Input::rumble(float low, float high, int ms) {
    if (m_pad && m_rumbleOn && m_gamepadActive)
        SDL_RumbleGamepad(m_pad, (Uint16)(clamp01(low) * 65535), (Uint16)(clamp01(high) * 65535), ms);
}

void Input::setLightbar(float r, float g, float b) {
    if (!m_pad || !m_hasLed) return;
    if (std::abs(r - m_led[0]) + std::abs(g - m_led[1]) + std::abs(b - m_led[2]) < 0.02f) return;
    m_led[0] = r; m_led[1] = g; m_led[2] = b;
    SDL_SetGamepadLED(m_pad, (Uint8)(clamp01(r) * 255), (Uint8)(clamp01(g) * 255), (Uint8)(clamp01(b) * 255));
}

} // namespace sv
