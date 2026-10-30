#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <array>

namespace sv {

enum class Act {
    Left, Right, Up, Down, Jump, Attack, Guard, Dash, Grapple, Shuriken, Art, Heal, Interact,
    Pause, Menu, Confirm, Cancel, TabPrev, TabNext, Count
};

// One tick of input. The simulation only sees this struct, so replays and tests can synthesize it.
struct InputFrame {
    std::array<bool, (size_t)Act::Count> down{};
    float moveX = 0, moveY = 0;   // analog, -1..1 (left stick, response-curved)
    float aimX = 0, aimY = 0;     // right stick: aiming and camera peek
    bool held(Act a) const { return down[(size_t)a]; }
};

class Input {
public:
    Input() { m_since.fill(1e9f); }
    void init();
    void shutdown();
    void handleEvent(const SDL_Event& e);
    // Builds the frame from device state; call once per fixed tick.
    void tick(float dt);
    void injectFrame(const InputFrame& f);   // overrides devices (autoplay/tests)
    void clearInjection() { m_injected = false; }

    bool held(Act a) const { return m_cur.held(a); }
    bool pressed(Act a) const { return m_cur.held(a) && !m_prev.held(a); }
    bool released(Act a) const { return !m_cur.held(a) && m_prev.held(a); }
    // Buffered press: true if pressed within `window` seconds and not yet consumed.
    bool buffered(Act a, float window) const { return m_since[(size_t)a] <= window; }
    void consume(Act a) { m_since[(size_t)a] = 1e9f; }
    float moveX() const { return m_cur.moveX; }
    float moveY() const { return m_cur.moveY; }
    const InputFrame& frame() const { return m_cur; }

    float aimX() const { return m_cur.aimX; }
    float aimY() const { return m_cur.aimY; }
    bool aiming() const { return m_cur.aimX * m_cur.aimX + m_cur.aimY * m_cur.aimY > 0.16f; }

    // The game is designed for a controller; keyboard & mouse is the fallback mapping.
    enum class Style { Keyboard, PlayStation, Xbox };
    Style style() const { return m_gamepadActive ? m_padStyle : Style::Keyboard; }
    Style padStyle() const { return m_padStyle; }   // even while the keyboard is in use
    bool hasPad() const { return m_pad != nullptr; }
    bool usingGamepad() const { return m_gamepadActive; }
    // Short text label for the current device (prefer drawing icons via ui/glyphs).
    std::string glyph(Act a) const;
    std::string keyLabel(Act a) const;
    void rumble(float low, float high, int ms);
    void setLightbar(float r, float g, float b);
    void setRumbleEnabled(bool on) { m_rumbleOn = on; }
    // Guard can be configured as toggle for accessibility.
    bool guardToggle = false;

private:
    InputFrame poll();
    InputFrame m_cur, m_prev;
    std::array<float, (size_t)Act::Count> m_since{};
    InputFrame m_injectedFrame;
    bool m_injected = false;
    SDL_Gamepad* m_pad = nullptr;
    Style m_padStyle = Style::PlayStation;
    bool m_hasLed = false;
    float m_led[3] = {-1, -1, -1};
    void detectPad();
    bool m_gamepadActive = false;
    bool m_rumbleOn = true;
    bool m_mouseL = false, m_mouseR = false;
    bool m_guardToggled = false;
    bool m_rawGuardPrev = false;
};

} // namespace sv
