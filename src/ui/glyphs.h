#pragma once
#include "core/input.h"
#include "core/math.h"
#include "render/text.h"
#include <string>

namespace sv {

class Renderer;

// Button prompts drawn as icons for the active device: PlayStation shapes, Xbox letters, or keycaps.
// Returns the icon's width. `center` is the icon's centre; `size` its height in current view units.
float drawGlyph(Renderer& r, const Input& in, Act a, Vec2 center, float size, float alpha = 1.f, Input::Style forced = (Input::Style)-1);
float glyphWidth(Renderer& r, const Input& in, Act a, float size, Input::Style forced = (Input::Style)-1);

// Text with inline {Action} tokens, e.g. "Tap {Guard} to deflect". Tokens: Jump Attack Guard Dash Grapple
// Shuriken Art Heal Interact Pause Menu Confirm Cancel Down Up Move Aim.
float drawRich(Renderer& r, const Input& in, Font f, float size, Vec2 pos, const std::string& text, Color col, Align align = Align::Left);
float measureRich(Renderer& r, const Input& in, Font f, float size, const std::string& text);

} // namespace sv
