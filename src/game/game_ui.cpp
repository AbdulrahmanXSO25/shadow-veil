#include "game/game.h"
#include "render/renderer.h"
#include "render/text.h"
#include "core/util.h"
#include "ui/glyphs.h"

namespace sv {

namespace col {
const Color paper = rgb(0xefe6d2);
const Color paperDim = rgb(0xa89e8a);
const Color crimson = rgb(0xb3121f);
const Color gold = rgb(0xd8b45a);
const Color ink = rgb(0x07070a);
const Color muted = rgb(0x6a6458);
}

// =================================================================== frame

void Game::render(float alpha) {
    Renderer& r = *m_r;
    World& w = *m_world;
    r.beginScene();
    w.render(r, alpha);
    PostParams post = w.palette.post;
    post.time = m_time;
    post.flash = w.flash; post.flashColor = w.flashColor;
    post.chroma = w.chroma * 0.6f;
    post.desaturate = w.desat * 0.85f;
    post.ripple = w.ripple;
    Vec2 camPos = lerp(w.cam.prevPos, w.cam.pos, alpha);
    Vec2 vs = r.viewSize();
    Vec2 rp = (w.rippleAt - camPos);
    post.rippleCenter = {0.5f + rp.x / vs.x, 0.5f - rp.y / vs.y};
    post.letterbox = m_cut.view().letterbox;
    post.fade = std::max(m_cut.view().fade, m_transition);
    if (m_screen == Screen::Title) post.fade = std::max(post.fade, 1 - m_titleFade);
    if (m_screen == Screen::Death) post.desaturate = std::min(1.f, 0.6f + m_deathT * 0.3f);
    r.endScene(post);
    if (m_clean) w.prompt.clear();   // cinematic capture: keep in-world telegraphs, drop button prompts
    if (m_screen == Screen::Playing || m_screen == Screen::Lore) w.renderWorldUI(r, alpha);
    r.setUIView();
    r.text().setPixelScale(r.uiScale());

    switch (m_screen) {
    case Screen::Title: drawTitle(); break;
    case Screen::Playing:
        if (m_clean) break;
        if (!m_cut.active() && m_transition < 0.5f) { drawHUD(); if (!m_quiet) drawHint(); }
        if (!m_quiet) drawRoomCard();
        drawDialogue();
        break;
    case Screen::Pause: drawHUD(); drawPause(); break;
    case Screen::Equipment: drawEquipment(); break;
    case Screen::Skills: drawSkills(); break;
    case Screen::Collection: drawCollection(); break;
    case Screen::Achievements: drawAchievements(); break;
    case Screen::Settings: drawSettings(); break;
    case Screen::Shrine: drawShrine(); break;
    case Screen::Forge: drawForge(); break;
    case Screen::Lore: drawLore(); break;
    case Screen::Death: drawDeath(); break;
    case Screen::ChapterEnd: drawChapterEnd(); break;
    case Screen::Credits: drawCredits(); break;
    case Screen::HowTo: drawHowTo(); break;
    case Screen::Chapters: drawChapters(); break;
    case Screen::Confirm:
        if (m_prevScreen == Screen::Title || m_confirmBack == Screen::Title) drawTitle();
        if (m_confirmBack == Screen::Chapters) drawChapters();
        drawConfirm();
        break;
    }
    if (!m_clean && !m_quiet) drawToasts();
    if (m_messageT > 0 && !m_clean) {
        TextSystem& T = r.text();
        float a = clamp01(m_messageT * 2);
        float wdt = T.measure(Font::Body, 22, m_message) + 80;
        r.textured(r.brushTex(), true, {r.uiWidth() / 2 - wdt / 2, 960, wdt, 54}, {0, 0, 1, 1}, col::ink.withA(0.85f * a));
        T.draw(r, Font::Body, 22, {r.uiWidth() / 2, 994}, m_message, col::paper.withA(a), Align::Center);
    }
    r.endFrame();
}

// =================================================================== helpers

void Game::drawPanel(const Rect& rc, float alpha) {
    Renderer& r = *m_r;
    r.rect(rc, col::ink.withA(alpha));
    r.rect({rc.x, rc.y, rc.w, 1.5f}, col::paperDim.withA(0.25f * alpha));
    r.rect({rc.x, rc.bottom() - 1.5f, rc.w, 1.5f}, col::paperDim.withA(0.25f * alpha));
}

void Game::drawSeal(Vec2 c, float size, const std::string& kanji, Color bg) {
    Renderer& r = *m_r;
    // a hanko seal: slightly irregular square, carved-looking glyph
    float s = size / 2;
    Vec2 q[4] = {c + Vec2{-s, -s * 0.96f}, c + Vec2{s * 0.97f, -s}, c + Vec2{s, s * 0.98f}, c + Vec2{-s * 0.98f, s}};
    r.polygon(q, 4, bg);
    r.text().draw(r, Font::Kanji, size * 0.62f, c + Vec2{0, size * 0.22f}, kanji, col::paper.withA(bg.a * 0.95f), Align::Center);
}

void Game::drawHeader(const std::string& kanji, const std::string& title, const std::string& sub) {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float x = 110;
    drawSeal({x, 112}, 74, kanji, col::crimson);
    T.draw(r, Font::Bold, 46, {x + 64, 118}, title, col::paper, Align::Left, 3);
    if (!sub.empty()) T.draw(r, Font::Italic, 20, {x + 66, 150}, sub, col::paperDim);
    r.textured(r.brushTex(), true, {x - 40, 170, 760, 14}, {0, 0, 1, 1}, col::paperDim.withA(0.25f));
}

void Game::drawMenuList(const std::vector<std::string>& items, int sel, Vec2 at, float size, const std::vector<bool>* enabled) {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float lh = size * 1.75f;
    for (size_t i = 0; i < items.size(); i++) {
        Vec2 p = at + Vec2{0, i * lh};
        bool on = (int)i == sel;
        bool en = !enabled || (*enabled)[i];
        if (on) {
            float w = T.measure(Font::Bold, size, items[i]) + 90;
            float wob = 1 + 0.02f * std::sin(m_time * 3);
            r.textured(r.brushTex(), true, {p.x - 40, p.y - size * 1.05f, w * wob, size * 1.5f}, {0, 0, 1, 1}, col::crimson.withA(0.88f));
        }
        Color c = on ? col::paper : (en ? col::paperDim : col::muted);
        T.draw(r, on ? Font::Bold : Font::Body, size, p, items[i], c, Align::Left, 1.5f);
    }
}

std::string Game::hintText(const std::string& id) const {
    if (id == "move") return "{Move} move — a light tilt walks silently     {Jump} jump — hold for height";
    if (id == "jump") return "Hold {Jump} to jump higher. A tap is a hop.";
    if (id == "walljump") return "Press into a wall to slide down it. {Jump} kicks off it.";
    if (id == "crouch") return "Hold {Down} to crouch. Crouched in tall grass, you cannot be seen.";
    if (id == "stealth") return "Reach an unaware foe from behind or above, then {Attack} to assassinate.";
    if (id == "attack") return "{Attack} strikes. Chain three cuts.";
    if (id == "deflect") return "Tap {Guard} the instant a blow lands to Deflect. Holding only blocks.";
    if (id == "posture") return "Deflects and strikes break posture. When the seal appears, {Attack} to Execute.";
    if (id == "perilous") return "危 Perilous attacks can't be guarded. Veil-Step {Dash} through them, or jump.";
    if (id == "dash") return "{Dash} Veil-Step: a short dash. You are untouchable while stepping.";
    if (id == "grapple") return "Hold {Grapple} near a ring to swing on the Veil-Thread. {Aim} picks the ring.";
    if (id == "shuriken") return "{Shuriken} throws a shuriken. {Aim} aims it. It interrupts archers.";
    if (id == "ember") return "{Art} Ember Art: spend Sparks for a burning crescent. Sparks come from hits and deflects.";
    if (id == "heal") return "{Heal} drinks from the healing gourd.";
    if (id == "bell") return "The iron catch hangs beyond any blade… but not beyond a thrown star. {Shuriken}";
    if (id == "rope") return "A single rope holds the log bridge upright. Cut it with {Attack}.";
    if (id == "camp") return "{Interact} at a camp to rest, learn arts and forge. Resting returns your foes.";
    if (id == "menu") return "{Menu} opens equipment.  {Pause} pauses.";
    if (id == "archer") return "Archers' arrows can be deflected straight back at them with {Guard}.";
    if (id == "gale") return "Gale Stance: swifter steps; finishing cuts loose wind blades. Wind arts await at camps.";
    // ---- Chapters II–VI
    if (id == "swim") return "In water, {Jump} strokes upward. At the surface, {Jump} climbs out. {Down} dives.";
    if (id == "lurker") return "Ripples on still water: something waits beneath. Approach with {Down} held, or strike first.";
    if (id == "sluice") return "{Interact} turns a sluice wheel. Opening a sluice can drain a flooded path.";
    if (id == "trial") return "A trial stone. {Interact} to begin: defeat every foe before time runs out to awaken a Stance.";
    if (id == "tidewave") return "危 A tide wave can't be guarded. Jump it or Veil-Step {Dash} through it.";
    if (id == "flooded") return "The arena is flooding: you move slower in the water. Fight from the dry ground.";
    if (id == "stillwater") return "Stillwater Stance: a perfect Deflect chills the attacker. Chilled foes are slow and their posture cannot recover.";
    if (id == "updraft") return "Rising wind carries you up. Steer out of it with {Move}; hold {Down} to sink.";
    if (id == "crumble") return "Weathered ledges give way a moment after you land. Keep moving. They return.";
    if (id == "duelist") return "Ridge duelists hold a high guard and answer with a fast draw-cut. Watch for the glint, deflect it, then punish.";
    if (id == "crow") return "Crow scouts leap high and throw kunai fans. Deflect the knives, then press in.";
    if (id == "thunder") return "Thunder Stance: an Execution calls lightning down on up to three nearby foes.";
    if (id == "beacon") return "A fire-watch beacon. If its light finds you, the whole garrison comes. Douse it with {Shuriken}.";
    if (id == "hound") return "Hounds track by scent: tall grass will not hide you from one. Stay well away, or strike first.";
    if (id == "gunner") return "危 A gunner's shot can't be guarded. When the red aim-line appears, Veil-Step {Dash} or break line of sight.";
    if (id == "shadow") return "Shadow Stance: hold {Down} and stay still to fade into shadow. Cloaked, you cannot be seen.";
    if (id == "layers") return "Veil-Step {Dash} to cross between the Bright and the Hollow. Stone that is only an outline becomes real on the other side.";
    if (id == "snare") return "Hollow threads bind you on contact. Mash {Attack} or {Jump} to tear free, or Deflect them.";
    if (id == "mirror") return "Mirror Stance: each Veil-Step leaves an afterimage that draws foes' attention and absorbs one shot.";
    if (id == "chase") return "The Veil is coming apart behind you. Run!";
    if (id == "sentinel") return "Thread Sentinels are slow and heavy. Their low sweeps are perilous: jump them.";
    if (id == "summon") return "The White Wolf calls his hounds. Thin the pack, or press the lord before they arrive.";
    return id;
}

// =================================================================== HUD

void Game::drawHUD() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    World& w = *m_world;
    const Player& p = w.player;
    float maxHp = maxHpOf(m_save);
    float hpF = clamp01(p.hp / maxHp);
    m_hpGhost = p.hp > m_hpGhost ? p.hp : damp(m_hpGhost, p.hp, 2.5f, kDt);
    float barW = 340 * (maxHp / 100.f);
    Vec2 o{64, 58};
    // crest
    drawSeal(o + Vec2{0, 18}, 48, "影", col::crimson);
    Rect bar{o.x + 40, o.y + 2, barW, 26};
    r.textured(r.brushTex(), true, {bar.x - 8, bar.y - 6, bar.w + 16, bar.h + 12}, {0, 0, 1, 1}, col::ink.withA(0.8f));
    float ghostF = clamp01(m_hpGhost / maxHp);
    r.textured(r.brushTex(), true, {bar.x, bar.y, bar.w * ghostF, bar.h}, {0, 0, ghostF, 1}, col::paper.withA(0.5f));
    Color hpC = hpF < 0.3f ? lerp(col::crimson, rgb(0xff4040), 0.5f + 0.5f * std::sin(m_time * 6)) : col::crimson;
    r.textured(r.brushTex(), true, {bar.x, bar.y, bar.w * hpF, bar.h}, {0, 0, hpF, 1}, hpC);
    // ki
    float ki = p.ki / 100.f;
    Rect kb{o.x + 42, o.y + 38, 260, 6};
    r.rect(kb, col::ink.withA(0.7f));
    r.rect({kb.x, kb.y, kb.w * ki, kb.h}, col::gold);
    if (m_save.has("ember")) {
        float cost = m_save.skills.count("veil_well") ? 25.f : 35.f;
        for (float c = cost; c <= 100; c += cost) r.rect({kb.x + kb.w * c / 100 - 1, kb.y - 3, 2, kb.h + 6}, col::paper.withA(0.5f));
        if (p.ki >= cost) { r.setBlend(Blend::Additive); r.glow({kb.x + kb.w * ki, kb.y + 3}, 18, col::gold.withA(0.5f + 0.3f * std::sin(m_time * 5))); r.setBlend(Blend::Alpha); }
    }
    // gourds
    float gx = o.x + 44, gy = o.y + 74;
    for (int i = 0; i < m_save.maxGourds; i++) {
        bool full = i < m_save.gourds;
        Vec2 c{gx + i * 26.f, gy};
        Color fill = full ? rgb(0x9a6a3a) : col::ink.withA(0.6f);
        r.circle(c + Vec2{0, 4}, 8, fill);
        r.circle(c + Vec2{0, -7}, 5.5f, fill);
        r.rect({c.x - 1.5f, c.y - 15, 3, 4}, fill);
        if (!full) { r.ring(c + Vec2{0, 4}, 7, 8, col::paperDim.withA(0.4f)); }
    }
    // shuriken
    float sx = gx + m_save.maxGourds * 26 + 22;
    for (int k = 0; k < 4; k++) {
        Vec2 d = rotate({0, -9}, k * PI / 2 + 0.3f);
        Vec2 c{sx, gy};
        r.tri(c + d, c + rotate(d, 0.7f) * 0.32f, c + rotate(d, -0.7f) * 0.32f, col::paperDim);
    }
    T.draw(r, Font::Bold, 20, {sx + 16, gy + 7}, strfmt("%d", m_save.shuriken), col::paper);
    if (m_save.has("ember")) drawSeal({sx + 64, gy}, 26, "炎", p.ki >= (m_save.skills.count("veil_well") ? 25.f : 35.f) ? rgb(0xc8501a) : col::muted);
    // mon
    if (m_monShowT > 0 || m_screen == Screen::Pause) {
        float a = m_screen == Screen::Pause ? 1 : clamp01(m_monShowT);
        float rx = r.uiWidth() - 64;
        T.draw(r, Font::Bold, 24, {rx, 74}, strfmt("%d", m_save.mon), col::paper.withA(a), Align::Right);
        r.circle({rx - T.measure(Font::Bold, 24, strfmt("%d", m_save.mon)) - 20, 66}, 9, col::gold.withA(a));
        r.rect({rx - T.measure(Font::Bold, 24, strfmt("%d", m_save.mon)) - 23, 63, 6, 6}, col::ink.withA(a));
    }
    // Kage Rank
    if (w.style > 4) {
        float rx = r.uiWidth() - 90, ry = 330;
        float pop = 1 + 0.35f * ease::outBack(w.stylePop);
        int rk = w.styleRank;
        Color rc = rk >= 5 ? rgb(0xff4a3a) : rk >= 4 ? col::gold : rk >= 3 ? rgb(0x9ff0e0) : col::paper;
        float idleFade = clamp01(1.4f - std::max(0.f, w.styleIdle - 2.f) * 0.3f);
        r.textured(r.brushTex(), true, {rx - 110, ry - 70, 170, 90}, {0, 0, 1, 1}, col::ink.withA(0.7f * idleFade));
        T.draw(r, Font::Bold, 66 * pop, {rx, ry}, World::rankLetter(rk), rc.withA(idleFade), Align::Center);
        T.draw(r, Font::Italic, 18, {rx, ry + 30}, World::rankName(rk), col::paperDim.withA(idleFade), Align::Center, 2);
        float lo = World::rankThreshold(rk), hi = World::rankThreshold(rk + 1);
        float f = clamp01((w.style - lo) / std::max(1.f, hi - lo));
        r.rect({rx - 60, ry + 44, 120, 4}, col::ink.withA(0.8f * idleFade));
        r.rect({rx - 60, ry + 44, 120 * f, 4}, rc.withA(idleFade));
    }
    // trial timer
    if (w.trialActive) {
        float left = std::max(0.f, w.trialLimit - w.trialT);
        float cx = r.uiWidth() / 2;
        bool urgent = left < 10;
        Color tc = urgent ? lerp(col::paper, rgb(0xff4a3a), 0.5f + 0.5f * std::sin(m_time * 10)) : rgb(0x9ff0e0);
        r.textured(r.brushTex(), true, {cx - 230, 70, 460, 74}, {0, 0, 1, 1}, col::ink.withA(0.85f));
        drawSeal({cx - 170, 106}, 44, "風", rgb(0x2a6a64));
        T.draw(r, Font::Bold, 34, {cx + 10, 118}, strfmt("%d:%04.1f", (int)left / 60, std::fmod(left, 60.f)), tc, Align::Center);
        T.draw(r, Font::Body, 18, {cx + 150, 116}, strfmt("%d left", w.trialRemaining()), col::paperDim, Align::Center);
    }
    // boss
    if (w.bossActive && w.bossIdx >= 0) {
        const Enemy& b = w.enemies[w.bossIdx];
        m_bossBarT = std::min(1.f, m_bossBarT + kDt * 1.5f);
        float a = m_bossBarT;
        float W = std::min(900.f, r.uiWidth() - 200);
        float cx = r.uiWidth() / 2, y = 1000;
        T.draw(r, Font::Kanji, 30, {cx - W / 2, y - 22}, b.def->kanji, col::crimson.withA(a));
        T.draw(r, Font::Bold, 22, {cx - W / 2 + 100, y - 24}, b.def->name, col::paper.withA(a), Align::Left, 2);
        for (int i = 0; i < b.def->deathblows; i++) {
            bool left = i < b.deathblows;
            Vec2 c{cx + W / 2 - 14 - i * 30.f, y - 30};
            r.circle(c, 10, left ? col::crimson.withA(a) : col::ink.withA(0.6f * a));
            r.ring(c, 10, 12, col::paperDim.withA(0.6f * a));
        }
        float hf = clamp01(b.hp / (b.def->hp * b.hpScale));
        r.rect({cx - W / 2, y, W, 12}, col::ink.withA(0.8f * a));
        r.rect({cx - W / 2, y, W * hf, 12}, rgb(0xc8c0b0, a));
        float pf = clamp01(b.posture / b.def->posture);
        if (pf > 0.01f) r.rect({cx - W / 2 * pf * 0.6f, y - 10, W * pf * 0.6f, 6}, lerp(rgb(0xffd060), rgb(0xff3a20), pf).withA(a));
    }
}

void Game::drawHint() {
    if (m_hintT <= 0 || m_hint.empty()) return;
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float a = clamp01(m_hintT) * clamp01((7 - m_hintT) * 3);
    std::string txt = hintText(m_hint);
    float w = measureRich(r, *m_in, Font::Body, 24, txt) + 120;
    float cx = r.uiWidth() / 2;
    r.textured(r.brushTex(), true, {cx - w / 2, 146, w, 74}, {0, 0, 1, 1}, col::ink.withA(0.84f * a));
    drawRich(r, *m_in, Font::Body, 24, {cx, 192}, txt, col::paper.withA(a), Align::Center);
    (void)T;
}

void Game::drawRoomCard() {
    if (m_roomCard.empty() || m_roomCardT > 4.5f || m_cut.active()) return;
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float t = m_roomCardT;
    float a = clamp01(t * 1.5f) * clamp01((4.5f - t) * 1.2f);
    float cx = r.uiWidth() / 2;
    T.draw(r, Font::Italic, 40, {cx, 300}, m_roomCard, col::paper.withA(a), Align::Center, 4);
    float lw = 300 * ease::outCubic(clamp01(t / 1.2f));
    r.rectGradH({cx - lw, 318, lw, 2}, col::paper.withA(0), col::paper.withA(0.6f * a));
    r.rectGradH({cx, 318, lw, 2}, col::paper.withA(0.6f * a), col::paper.withA(0));
}

void Game::drawToasts() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float y = 130;
    for (auto& t : m_toasts) {
        float life = t.achievement ? 5.5f : 3.8f;
        float a = clamp01(t.t * 4) * clamp01((life - t.t) * 2);
        float slide = (1 - ease::outCubic(clamp01(t.t * 3))) * 60;
        float w = std::max(T.measure(Font::Bold, 22, t.title), T.measure(Font::Body, 17, t.sub)) + (t.achievement ? 120 : 70);
        float x = r.uiWidth() - 40 - w + slide;
        r.textured(r.brushTex(), true, {x - 20, y - 10, w + 40, t.sub.empty() ? 52.f : 80.f}, {0, 0, 1, 1}, col::ink.withA(0.85f * a));
        float tx = x + 20;
        if (t.achievement) { drawSeal({x + 34, y + 24}, 44, "誉", col::gold.withA(a)); tx = x + 70; }
        T.draw(r, Font::Bold, 22, {tx, y + 22}, t.title, (t.achievement ? col::gold : col::paper).withA(a));
        if (!t.sub.empty()) T.draw(r, Font::Body, 17, {tx, y + 50}, t.sub, col::paperDim.withA(a));
        y += t.sub.empty() ? 62 : 92;
    }
}

// =================================================================== dialogue & cinematic text

void Game::drawDialogue() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    const CutsceneView& v = m_cut.view();
    float sc = m_settings.subtitleScale;
    float cx = r.uiWidth() / 2;
    auto revealed = [&](const std::string& s) {
        auto cps = decodeUtf8(s);
        size_t n = std::min(cps.size(), (size_t)v.textReveal);
        std::string out;
        size_t bytes = 0, count = 0;
        while (bytes < s.size() && count < n) {
            unsigned char c = s[bytes];
            int len = c < 0x80 ? 1 : ((c >> 5) == 6 ? 2 : ((c >> 4) == 14 ? 3 : 4));
            bytes += len; count++;
        }
        out = s.substr(0, bytes);
        return out;
    };
    if (!v.text.empty()) {
        float W = std::min(1240.f, r.uiWidth() - 120);
        float y = 1080 - 150 - 40 * (sc - 1);
        auto lines = T.wrap(Font::Body, 30 * sc, v.text, W - 120);
        float h = 70 + lines.size() * 42 * sc;
        r.textured(r.brushTex(), true, {cx - W / 2 - 30, y - 40, W + 60, h + 20}, {0, 0, 1, 1}, col::ink.withA(0.9f));
        // speaker plate
        std::string sp = v.speaker;
        struct Who { const char* id; const char* kanji; const char* name; uint32_t seal; };
        static const Who kWho[] = {
            {"REI", "玲", "Rei", 0xb3202e}, {"ODAKA", "師", "Odaka", 0x3a4a7a}, {"FUKI", "婆", "Fuki", 0x8a6a2a},
            {"TETSU", "鉄", "Tetsu", 0x5a5a62}, {"SOLDIER", "兵", "Soldier", 0x3a4a7a}, {"HANA", "花", "Hana", 0xc0607a},
            {"SAYO", "織", "Sayo", 0xc8a040}, {"FUNAKI", "潮", "Funaki", 0x2a6a7a}, {"KARASU", "烏", "Karasu", 0x2a2a34},
            {"HAKURO", "白", "Hakuro", 0x8a94a8}, {"KAGAMI", "鏡", "Kagami", 0x6a5a9a}, {"FERRYMAN", "渡", "Ferryman", 0x3a5a5a},
            {"EXILE", "民", "Exile", 0x4a4a5a}, {"CAPTAIN", "将", "Captain", 0x6a3a2a},
        };
        const Who* who = nullptr;
        for (auto& k : kWho) if (sp == k.id) who = &k;
        std::string kanji = who ? who->kanji : "声";
        Color sealC = who ? rgb(who->seal) : rgb(0x3a4a7a);
        drawSeal({cx - W / 2 + 40, y - 6}, 50, kanji, sealC);
        std::string name = who ? who->name : sp;
        T.draw(r, Font::Bold, 24 * sc, {cx - W / 2 + 80, y + 2}, name, col::gold, Align::Left, 2);
        // typewriter reveal across wrapped lines
        std::string shown = revealed(v.text);
        auto shownLines = T.wrap(Font::Body, 30 * sc, shown, W - 120);
        for (size_t i = 0; i < shownLines.size() && i < lines.size(); i++)
            T.draw(r, Font::Body, 30 * sc, {cx - W / 2 + 40, y + 50 + i * 42 * sc}, shownLines[i], col::paper);
        if (v.waitingForConfirm) {
            float bob = std::sin(m_time * 5) * 3;
            T.draw(r, Font::Body, 22, {cx + W / 2 - 10, y + h - 30 + bob}, "›", col::gold, Align::Right);
        }
    }
    if (!v.narration.empty()) {
        float W = std::min(1100.f, r.uiWidth() - 160);
        auto lines = T.wrap(Font::Italic, 34 * sc, revealed(v.narration), W);
        auto full = T.wrap(Font::Italic, 34 * sc, v.narration, W);
        float y0 = 540 - full.size() * 26 * sc;
        for (size_t i = 0; i < lines.size(); i++) T.draw(r, Font::Italic, 34 * sc, {cx, y0 + i * 52 * sc}, lines[i], col::paper, Align::Center);
    }
    if (v.titleDur > 0) {
        float t = v.titleT, d = v.titleDur;
        float a = clamp01(t * 1.2f) * clamp01((d - t) * 1.2f);
        float spread = ease::outCubic(clamp01(t / 1.5f));
        T.draw(r, Font::Kanji, 110, {cx, 520}, v.titleKanji, col::paper.withA(a), Align::Center, 30 * (1 - spread) + 10);
        T.draw(r, Font::Body, 30, {cx, 600}, v.titleText, col::paperDim.withA(a), Align::Center, 10 * spread + 2);
        float lw = 260 * spread;
        r.rect({cx - lw, 622, lw * 2, 1.5f}, col::crimson.withA(a));
    }
    if (!v.choices.empty()) {
        // a decision: prompt above, options stacked, locked ones greyed with their reason implied
        float y = 470 - v.choices.size() * 34;
        r.rect({0, y - 90, r.uiWidth(), 140.f + v.choices.size() * 70}, col::ink.withA(0.72f));
        if (!v.choicePrompt.empty()) T.draw(r, Font::Italic, 34 * sc, {cx, y - 40}, v.choicePrompt, col::paper, Align::Center);
        for (size_t i = 0; i < v.choices.size(); i++) {
            bool sel = (int)i == v.choiceSel, locked = v.choiceLocked[i];
            float yy = y + 30 + i * 70;
            if (sel) {
                r.textured(r.brushTex(), true, {cx - 380, yy - 8, 760, 58}, {0, 0, 1, 1}, col::crimson.withA(0.55f));
                T.draw(r, Font::Body, 26, {cx - 340 + std::sin(m_time * 6) * 3, yy + 6}, "›", col::gold, Align::Left);
            }
            Color c = locked ? col::paperDim.withA(0.35f) : (sel ? col::paper : col::paperDim);
            T.draw(r, Font::Bold, 28 * sc, {cx, yy + 4}, locked ? v.choices[i] + "  (sealed)" : v.choices[i], c, Align::Center, 1);
        }
    }
    if (m_cut.active() && m_in->held(Act::Pause)) {
        drawRich(r, *m_in, Font::Body, 18, {r.uiWidth() - 40, 1050}, "Hold {Pause} to skip…", col::paperDim, Align::Right);
    }
}

// =================================================================== title

void Game::drawTitle() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float a = ease::inOutQuad(clamp01(m_titleFade * 1.4f - 0.2f));
    float W = r.uiWidth();
    // left-side vignette for legibility
    r.rectGradH({0, 0, 760, 1080}, col::ink.withA(0.78f * a), col::ink.withA(0));
    float x = 130;
    // vertical kanji title, brushed
    const char* kanji[] = {"影", "ノ", "帳"};
    for (int i = 0; i < 3; i++) {
        float ka = clamp01((m_titleFade - 0.25f - i * 0.12f) * 2.5f);
        T.draw(r, Font::Kanji, 112, {x, 250 + i * 122.f}, kanji[i], col::paper.withA(ka));
    }
    drawSeal({x + 54, 590}, 50, "玲", col::crimson.withA(a));
    T.draw(r, Font::Bold, 66, {x + 150, 300}, "SHADOW", col::paper.withA(a), Align::Left, 18);
    T.draw(r, Font::Bold, 66, {x + 150, 380}, "VEIL", col::paper.withA(a), Align::Left, 18);
    {
        // the subtitle follows the journey: the chapter your save is in
        T.draw(r, Font::Italic, 22, {x + 154, 424}, m_titleSub, col::paperDim.withA(a), Align::Left, 2);
    }
    r.rect({x + 154, 444, 300 * a, 1.5f}, col::crimson.withA(a));
    std::vector<std::string> items;
    if (hasSaveGame()) items.push_back("Continue");
    items.push_back("New Game");
    if (m_profile.ngPlusUnlocked) items.push_back("New Game+");
    if (m_profile.chaptersCleared >= 1) items.push_back("Chapters");
    for (auto s : {"How to Play", "Settings", "Achievements", "Credits", "Quit"}) items.push_back(s);
    if (m_screen == Screen::Title) drawMenuList(items, m_sel, {x + 160, 540}, 30);
    T.draw(r, Font::Body, 16, {W - 40, 1050}, "v" SV_VERSION, col::muted.withA(a), Align::Right);
    drawRich(r, *m_in, Font::Body, 17, {40, 1050}, "{Up}{Down} choose     {Confirm} select     {Cancel} back", col::paperDim.withA(a));
}

// =================================================================== pause & menus

void Game::drawPause() {
    Renderer& r = *m_r;
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.55f));
    r.rectGradH({0, 0, 700, 1080}, col::ink.withA(0.8f), col::ink.withA(0));
    drawHeader("止", "Paused", m_world->level.rooms[m_world->curRoom].name);
    drawMenuList({"Resume", "Equipment", "Way of the Shadow", "Collection", "Achievements", "How to Play", "Settings", "Quit to Title"}, m_sel, {130, 290}, 28);
    TextSystem& T = r.text();
    int mins = (int)(m_save.playTime / 60), secs = (int)m_save.playTime % 60;
    T.draw(r, Font::Body, 20, {130, 1000}, strfmt("Time %d:%02d   ·   Kills %d   ·   Deflects %d   ·   Deaths %d", mins, secs, m_save.counter("kills"), m_save.counter("deflects"), m_save.counter("deaths")), col::paperDim);
    if (m_save.ngPlus > 0) T.draw(r, Font::Bold, 20, {130, 1030}, strfmt("New Game+%d", m_save.ngPlus), col::gold);
}

void Game::drawConfirm() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.6f));
    float cx = r.uiWidth() / 2;
    auto lines = T.wrap(Font::Body, 28, m_confirmText, 800);
    r.textured(r.brushTex(), true, {cx - 520, 380, 1040, 300}, {0, 0, 1, 1}, col::ink.withA(0.95f));
    for (size_t i = 0; i < lines.size(); i++) T.draw(r, Font::Body, 28, {cx, 460 + i * 40.f}, lines[i], col::paper, Align::Center);
    const char* opts[] = {"Yes", "No"};
    for (int i = 0; i < 2; i++) {
        Vec2 p{cx + (i == 0 ? -120.f : 120.f), 610};
        if (m_sel == i) r.textured(r.brushTex(), true, {p.x - 70, p.y - 34, 140, 48}, {0, 0, 1, 1}, col::crimson.withA(0.9f));
        T.draw(r, Font::Bold, 28, p, opts[i], m_sel == i ? col::paper : col::paperDim, Align::Center);
    }
}

void Game::drawStatsBlock(Vec2 at) {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    Stats s = computeStats(m_save);
    struct Row { const char* k; std::string v; };
    Row rows[] = {
        {"Attack", strfmt("%.0f", s.attack)},
        {"Vitality", strfmt("%.0f", maxHpOf(m_save))},
        {"Defense", strfmt("%+.0f%%", s.defense * 100)},
        {"Posture damage", strfmt("%+.0f%%", s.postureDmg * 100)},
        {"Deflect window", strfmt("%+.0f%%", s.deflectWindow * 100)},
        {"Stealth", strfmt("%+.0f%%", s.stealth * 100)},
        {"Spark gain", strfmt("%+.0f%%", s.kiGain * 100)},
        {"Blade temper", strfmt("%d / %d", m_save.bladeLevel, kMaxBladeLevel)},
    };
    for (size_t i = 0; i < 8; i++) {
        T.draw(r, Font::Body, 20, at + Vec2{0, i * 30.f}, rows[i].k, col::paperDim);
        T.draw(r, Font::Bold, 20, at + Vec2{300, i * 30.f}, rows[i].v, col::paper, Align::Right);
    }
}

void Game::drawEquipment() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.86f));
    drawHeader("装", "Equipment", m_atShrine ? "At the camp" : "Choose what you carry");
    const ItemDef* blade = findItem(m_save.blade);
    int sockets = blade ? blade->sockets : 0;
    auto gemAt = [&](int i) -> std::string {
        auto it = m_save.gems.find(m_save.blade);
        if (it == m_save.gems.end() || (int)it->second.size() <= i) return "";
        return it->second[i];
    };
    std::string cur[7] = {m_save.blade, gemAt(0), gemAt(1), m_save.garb, m_save.mask, m_save.relic1, m_save.relic2};
    float x = 130, y = 270;
    for (int i = 0; i < 7; i++) {
        bool on = i == m_sel;
        Vec2 p{x, y + i * 70.f};
        if (on) r.textured(r.brushTex(), true, {p.x - 40, p.y - 40, 620, 62}, {0, 0, 1, 1}, (m_listMode ? col::muted : col::crimson).withA(0.85f));
        T.draw(r, Font::Body, 18, p + Vec2{0, -14}, kSlotRows[i], col::paperDim, Align::Left, 2);
        bool socketless = (i == 1 && sockets < 1) || (i == 2 && sockets < 2);
        const ItemDef* d = findItem(cur[i]);
        std::string name = socketless ? "— no socket —" : (d ? d->name : "(empty)");
        T.draw(r, Font::Bold, 26, p + Vec2{0, 14}, name, socketless ? col::muted : col::paper);
        if (d) drawSeal({p.x + 520, p.y}, 40, d->kanji, i == 0 ? col::crimson : rgb(0x3a3a48));
    }
    // candidate list / details
    float rx = std::max(760.f, r.uiWidth() * 0.5f);
    std::string focus = cur[m_sel];
    if (m_listMode) {
        auto list = candidates(m_save, m_sel);
        for (size_t i = 0; i < list.size(); i++) {
            const ItemDef* d = findItem(list[i]);
            Vec2 p{rx, 290 + i * 48.f};
            bool on = (int)i == m_sel2;
            if (on) r.textured(r.brushTex(), true, {p.x - 30, p.y - 32, 520, 46}, {0, 0, 1, 1}, col::crimson.withA(0.85f));
            std::string n = d ? d->name : "(remove)";
            if (d && d->slot == ItemSlot::Gem) n += strfmt("  ×%d", m_save.count(list[i]));
            T.draw(r, on ? Font::Bold : Font::Body, 24, p, n, on ? col::paper : col::paperDim);
        }
        if (m_sel2 < (int)list.size()) focus = list[m_sel2];
    }
    const ItemDef* fd = findItem(focus);
    float dy = m_listMode ? 640 : 290;
    if (fd) {
        drawSeal({rx + 30, dy + 10}, 60, fd->kanji, col::crimson);
        T.draw(r, Font::Bold, 30, {rx + 80, dy + 18}, fd->name, col::paper);
        T.draw(r, Font::Body, 18, {rx + 82, dy + 44}, std::string(slotName(fd->slot)) + (fd->sockets ? strfmt(" · %d socket%s", fd->sockets, fd->sockets > 1 ? "s" : "") : ""), col::gold);
        auto dl = T.wrap(Font::Body, 22, fd->desc, 600);
        for (size_t i = 0; i < dl.size(); i++) T.draw(r, Font::Body, 22, {rx, dy + 90 + i * 30.f}, dl[i], col::paper);
        auto ll = T.wrap(Font::Italic, 20, fd->lore, 600);
        for (size_t i = 0; i < ll.size(); i++) T.draw(r, Font::Italic, 20, {rx, dy + 110 + dl.size() * 30 + i * 28.f}, ll[i], col::paperDim);
    }
    if (!m_listMode) drawStatsBlock({rx, 700});
    drawRich(r, *m_in, Font::Body, 18, {130, 1040}, m_listMode ? "{Confirm} equip     {Cancel} back" : "{Confirm} change     {Cancel} close", col::paperDim);
}

void Game::drawSkills() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.88f));
    drawHeader("道", "Way of the Shadow", m_atShrine ? "Rest to learn" : "Learn new arts at a camp");
    T.draw(r, Font::Bold, 24, {r.uiWidth() - 100, 118}, strfmt("Skill points  %d", m_save.skillPoints), col::gold, Align::Right);
    float ef = m_save.essence / (float)kEssencePerPoint;
    r.rect({r.uiWidth() - 400, 134, 300, 4}, col::ink.withA(0.8f));
    r.rect({r.uiWidth() - 400, 134, 300 * ef, 4}, col::gold.withA(0.7f));
    float W = r.uiWidth();
    float colW = std::min(400.f, (W - 200) / 4);
    float x0 = W / 2 - colW * 2.f;
    const SkillDef* focus = nullptr;
    for (int b = 0; b < 4; b++) {
        float cx = x0 + colW * (b + 0.5f);
        bool sealed = b == 3 && !m_save.has("gale");
        T.draw(r, Font::Bold, 26, {cx, 250}, branchName(b), sealed ? col::muted : (b == 3 ? rgb(0x9ff0e0) : col::paperDim), Align::Center, 4);
        if (sealed) T.draw(r, Font::Italic, 16, {cx, 276}, "sealed — awaken a Stance", col::muted, Align::Center);
        r.rect({cx - 1, 300, 2, 450}, col::paperDim.withA(0.2f));
        for (auto& d : skillDefs()) {
            if (d.branch != b) continue;
            Vec2 c{cx, 330 + d.tier * 142.f};
            bool learned = m_save.skills.count(d.id) > 0;
            bool on = m_col == b && m_sel == d.tier;
            bool avail = canLearn(m_save, d);
            if (on) { focus = &d; r.setBlend(Blend::Additive); r.glow(c, 110, col::crimson.withA(0.4f)); r.setBlend(Blend::Alpha); }
            r.circle(c, 42, col::ink);
            r.ring(c, 40, 44, learned ? col::gold : (avail ? col::paper.withA(0.7f) : col::muted));
            T.draw(r, Font::Kanji, 38, c + Vec2{0, 14}, d.kanji, learned ? col::gold : (avail ? col::paper : col::muted), Align::Center);
            T.draw(r, on ? Font::Bold : Font::Body, 19, c + Vec2{0, 70}, d.name, on ? col::paper : col::paperDim, Align::Center);
        }
    }
    if (focus) {
        float y = 930;
        T.draw(r, Font::Body, 24, {W / 2, y}, focus->desc, col::paper, Align::Center);
        std::string why;
        bool ok = canLearn(m_save, *focus, &why);
        drawRich(r, *m_in, Font::Italic, 20, {W / 2, y + 40}, m_save.skills.count(focus->id) ? "Learned." : (ok ? (m_atShrine ? "{Confirm} learn" : "Rest at a camp to learn.") : why), col::gold, Align::Center);
    }
}

void Game::drawCollection() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.88f));
    int found = 0;
    for (auto& l : loreDefs()) if (m_save.has("lore:" + l.id)) found++;
    drawHeader("巻", "Collection", strfmt("Ink Scrolls %d / %zu   ·   Resin Beads %d", found, loreDefs().size(), m_save.beads));
    // chapter tabs
    int ch = m_col + 1;
    auto& chs = chapterDefs();
    float tx = 130;
    for (auto& c : chs) {
        bool on = c.number == ch;
        int got = 0, tot = 0;
        for (auto& l : loreDefs()) if (l.chapter == c.number) { tot++; if (m_save.has("lore:" + l.id)) got++; }
        std::string lab = strfmt("%s  %d/%d", c.numeral.c_str(), got, tot);
        float w = T.measure(Font::Body, 20, lab) + 40;
        if (on) r.textured(r.brushTex(), true, {tx - 14, 216, w, 42}, {0, 0, 1, 1}, col::crimson.withA(0.75f));
        T.draw(r, on ? Font::Bold : Font::Body, 20, {tx + 6, 244}, lab, on ? col::paper : col::paperDim);
        tx += w + 16;
    }
    drawRich(r, *m_in, Font::Body, 16, {tx + 20, 244}, "{TabPrev}{TabNext}", col::paperDim);
    std::vector<const LoreDef*> list;
    for (auto& l : loreDefs()) if (l.chapter == ch) list.push_back(&l);
    for (size_t i = 0; i < list.size(); i++) {
        bool has = m_save.has("lore:" + list[i]->id);
        bool on = (int)i == m_sel;
        Vec2 p{130, 330 + i * 60.f};
        if (on) r.textured(r.brushTex(), true, {p.x - 40, p.y - 38, 560, 54}, {0, 0, 1, 1}, col::crimson.withA(0.85f));
        T.draw(r, on ? Font::Bold : Font::Body, 24, p, has ? list[i]->title : "? ? ?", has ? col::paper : col::muted);
    }
    // materials
    float my = 330 + 5 * 60 + 60;
    T.draw(r, Font::Bold, 22, {130, my}, "Materials", col::paperDim, Align::Left, 2);
    int k = 0;
    for (auto& [id, n] : m_save.inventory) {
        const ItemDef* d = findItem(id);
        if (!d || d->slot != ItemSlot::Material) continue;
        T.draw(r, Font::Body, 22, {130, my + 40 + k * 32.f}, strfmt("%s  ×%d", d->name.c_str(), n), col::paper);
        k++;
    }
    if (k == 0) T.draw(r, Font::Italic, 20, {130, my + 40}, "None yet.", col::muted);
    if (m_sel < (int)list.size() && m_save.has("lore:" + list[m_sel]->id)) {
        float rx = 760, pw = std::min(900.f, r.uiWidth() - rx - 100);
        r.rect({rx - 30, 250, pw + 60, 640}, rgb(0xe8dcc0, 0.92f));
        T.draw(r, Font::Bold, 30, {rx, 310}, list[m_sel]->title, col::ink);
        auto lines = T.wrap(Font::Italic, 24, list[m_sel]->text, pw);
        for (size_t i = 0; i < lines.size(); i++) T.draw(r, Font::Italic, 24, {rx, 370 + i * 36.f}, lines[i], rgb(0x2a2420));
        drawSeal({rx + pw - 10, 840}, 50, "墨", col::crimson);
    }
}

void Game::drawAchievements() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.9f));
    auto& defs = achievementDefs();
    int got = 0;
    for (auto& a : defs) if (m_profile.achievements.count(a.id)) got++;
    drawHeader("誉", "Achievements", strfmt("%d of %zu earned", got, defs.size()));
    int perPage = 10;
    int first = std::max(0, std::min(m_sel - perPage / 2, (int)defs.size() - perPage));
    for (int i = first; i < std::min((int)defs.size(), first + perPage); i++) {
        auto& a = defs[i];
        bool has = m_profile.achievements.count(a.id) > 0;
        bool on = i == m_sel;
        Vec2 p{180, 270 + (i - first) * 72.f};
        if (on) r.textured(r.brushTex(), true, {p.x - 90, p.y - 40, 1100, 70}, {0, 0, 1, 1}, col::crimson.withA(0.6f));
        drawSeal({p.x - 40, p.y - 6}, 40, has ? "誉" : "・", has ? col::gold : col::muted);
        bool hide = a.hidden && !has;
        T.draw(r, Font::Bold, 24, p, hide ? "Hidden" : a.name, has ? col::paper : col::paperDim);
        T.draw(r, Font::Body, 18, p + Vec2{0, 24}, hide ? "Keep exploring." : a.desc, col::paperDim);
    }
}

void Game::drawSettings() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.9f));
    drawHeader("設", "Settings", "Audio · Accessibility · Display");
    const Settings& s = m_settings;
    auto pct = [](float v) { return strfmt("%d%%", (int)std::round(v * 100)); };
    auto onoff = [](bool b) { return std::string(b ? "On" : "Off"); };
    std::vector<std::pair<std::string, std::string>> rows = {
        {"Master volume", pct(s.master)}, {"Music", pct(s.music)}, {"Effects", pct(s.sfx)},
        {"Screen shake", pct(s.shake)}, {"Reduce flashes", onoff(s.reduceFlashes)},
        {"Deflect assist", strfmt("×%.2f window", s.deflectAssist)}, {"Game speed", pct(s.gameSpeed)},
        {"Guard", s.guardToggle ? "Toggle" : "Hold"}, {"Subtitle size", pct(s.subtitleScale)},
        {"High-contrast foes", onoff(s.highContrast)}, {"Blood effects", s.inkSplatter ? "Ink" : "Petals"},
        {"Tutorial hints", onoff(s.hints)}, {"Controller rumble", onoff(s.rumble)},
        {"Fullscreen", onoff(s.fullscreen)}, {"V-Sync", onoff(s.vsync)},
    };
    for (size_t i = 0; i < rows.size(); i++) {
        bool on = (int)i == m_sel;
        Vec2 p{130, 250 + i * 50.f};
        if (on) r.textured(r.brushTex(), true, {p.x - 40, p.y - 34, 880, 48}, {0, 0, 1, 1}, col::crimson.withA(0.85f));
        T.draw(r, on ? Font::Bold : Font::Body, 24, p, rows[i].first, on ? col::paper : col::paperDim);
        T.draw(r, Font::Bold, 24, p + Vec2{780, 0}, (on ? "‹  " : "") + rows[i].second + (on ? "  ›" : ""), col::paper, Align::Right);
    }
    drawRich(r, *m_in, Font::Body, 18, {130, 1040}, "{Left}{Right} adjust     {Cancel} save & back", col::paperDim);
}

void Game::drawShrine() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rectGradH({0, 0, 900, 1080}, col::ink.withA(0.85f), col::ink.withA(0));
    std::string name = m_world->level.rooms[m_world->curRoom].name;
    drawHeader("火", "Camp", name);
    drawMenuList({"Rest", "Meditate", "Forge", "Equipment", "Leave"}, m_sel, {130, 300}, 30);
    const char* desc[] = {"Restore vitality and gourds. Your foes return.", "Spend skill points on the Way of the Shadow.",
                          "Temper blades and forge gems, straps and shuriken.", "Change blades, garb, masks and tokens.", "Return to the path."};
    T.draw(r, Font::Italic, 22, {130, 620}, desc[m_sel], col::paperDim);
    T.draw(r, Font::Body, 20, {130, 680}, strfmt("Mon %d    Skill points %d    Gourds %d/%d", m_save.mon, m_save.skillPoints, m_save.gourds, m_save.maxGourds), col::gold);
}

void Game::drawForge() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.88f));
    drawHeader("鍛", "Forge", strfmt("Mon %d", m_save.mon));
    auto& rec = recipeDefs();
    for (size_t i = 0; i < rec.size(); i++) {
        bool on = (int)i == m_sel;
        bool ok = canCraft(m_save, rec[i]);
        Vec2 p{130, 280 + i * 84.f};
        if (on) r.textured(r.brushTex(), true, {p.x - 40, p.y - 40, 1100, 80}, {0, 0, 1, 1}, col::crimson.withA(0.8f));
        T.draw(r, Font::Bold, 26, p, rec[i].name, ok ? col::paper : col::paperDim);
        std::string cost;
        int mon = rec[i].output == "@temper" ? rec[i].mon * (m_save.bladeLevel + 1) : rec[i].mon;
        for (auto& [id, n] : rec[i].inputs) {
            const ItemDef* d = findItem(id);
            cost += strfmt("%s %d/%d   ", d ? d->name.c_str() : id.c_str(), m_save.count(id), n);
        }
        cost += strfmt("Mon %d", mon);
        T.draw(r, Font::Body, 18, p + Vec2{0, 28}, rec[i].desc + "    —    " + cost, ok ? col::gold : col::muted);
    }
}

void Game::drawLore() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    const LoreDef* l = findLore(m_loreId);
    if (!l) return;
    float a = ease::outCubic(clamp01(m_screenT * 2.5f));
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.6f * a));
    float W = std::min(900.f, r.uiWidth() - 160), cx = r.uiWidth() / 2;
    float y = 200 + (1 - a) * 40;
    r.rect({cx - W / 2 - 40, y, W + 80, 640}, rgb(0xe8dcc0, 0.95f * a));
    r.rect({cx - W / 2 - 50, y - 10, W + 100, 14}, rgb(0x2a1a10, a));
    r.rect({cx - W / 2 - 50, y + 636, W + 100, 14}, rgb(0x2a1a10, a));
    T.draw(r, Font::Body, 18, {cx, y + 60}, "INK SCROLL", rgb(0x7a1a1a, a), Align::Center, 6);
    T.draw(r, Font::Bold, 34, {cx, y + 110}, l->title, rgb(0x14100c, a), Align::Center);
    auto lines = T.wrap(Font::Italic, 26, l->text, W - 40);
    for (size_t i = 0; i < lines.size(); i++) T.draw(r, Font::Italic, 26, {cx, y + 180 + i * 40.f}, lines[i], rgb(0x2a2420, a), Align::Center);
    drawSeal({cx + W / 2 - 20, y + 580}, 54, "墨", col::crimson.withA(a));
}

void Game::drawDeath() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float t = m_deathT;
    float a = ease::outCubic(clamp01(t / 1.2f));
    float cx = r.uiWidth() / 2;
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.4f * a));
    T.draw(r, Font::Kanji, 260 + 30 * (1 - a), {cx, 600}, "死", col::crimson.withA(a * 0.95f), Align::Center);
    T.draw(r, Font::Body, 28, {cx, 700}, "The thread is cut — for now.", col::paper.withA(a), Align::Center, 3);
    if (t > 1.6f) drawRich(r, *m_in, Font::Body, 22, {cx, 770}, "{Confirm} return to the camp", col::paperDim.withA(clamp01(t - 1.6f)), Align::Center);
}

void Game::drawChapterEnd() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    float t = m_screenT;
    float a = ease::outCubic(clamp01(t / 2));
    float cx = r.uiWidth() / 2;
    const EndCard& E = m_end;
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.9f * a));
    T.draw(r, Font::Kanji, 96, {cx, 250}, E.numeral + " 終", col::paper.withA(a), Align::Center, 10);
    std::string head = E.final ? "The End — " + E.title : strfmt("End of Chapter %d — %s", E.number, E.title.c_str());
    T.draw(r, Font::Body, 28, {cx, 310}, head, col::paperDim.withA(a), Align::Center, 3);
    if (E.final && E.ending > 0) {
        static const char* names[] = {"", "Ending: Kept", "Ending: Unbound", "Ending: The Second Way"};
        T.draw(r, Font::Bold, 24, {cx, 350}, names[E.ending], col::gold.withA(a), Align::Center, 4);
    }
    for (size_t i = 0; i < E.rows.size(); i++) {
        float ra = clamp01((t - 0.8f - i * 0.15f) * 3);
        T.draw(r, Font::Body, 24, {cx - 20, 400 + i * 40.f}, E.rows[i].first, col::paperDim.withA(ra), Align::Right);
        T.draw(r, Font::Bold, 24, {cx + 20, 400 + i * 40.f}, E.rows[i].second, col::paper.withA(ra));
    }
    {
        const ChapterResult& res = E.res;
        float ga = clamp01((t - 2.0f) * 2);
        float gp = 1 + 0.4f * (1 - ease::outBack(clamp01((t - 2.0f) * 2)));
        T.draw(r, Font::Bold, 110 * gp, {cx + 470, 560}, res.grade, (res.grade[0] == 'S' ? col::gold : col::paper).withA(ga), Align::Center);
        T.draw(r, Font::Body, 22, {cx + 470, 600}, strfmt("Score %d", res.score), col::paperDim.withA(ga), Align::Center);
        bool record = res.score > E.prevBest && res.score > 0;
        if (record) T.draw(r, Font::Bold, 20, {cx + 470, 634}, "NEW RECORD", col::crimson.withA(ga * (0.6f + 0.4f * std::sin(m_time * 6))), Align::Center, 4);
        else T.draw(r, Font::Body, 18, {cx + 470, 634}, strfmt("Best %d (%s)", E.prevBest, E.prevGrade.c_str()), col::paperDim.withA(ga), Align::Center);
        T.draw(r, Font::Body, 18, {cx + 470, 664}, strfmt("Highest Kage Rank: %s", World::rankLetter(m_save.counter("maxRank"))), col::paperDim.withA(ga), Align::Center);
    }
    float ba = clamp01(t - 2.5f);
    if (!E.final) T.draw(r, Font::Italic, 22, {cx, 730}, "Next: " + E.nextTitle, col::gold.withA(ba), Align::Center);
    else T.draw(r, Font::Italic, 22, {cx, 730}, "Thank you for walking the whole thread.", col::gold.withA(ba), Align::Center);
    T.draw(r, Font::Body, 20, {cx, 770}, E.final ? "New Game+ and Chapter select are open from the title screen."
                                                 : "Your journey is saved. Chapters can be replayed from the title screen.", col::paperDim.withA(ba), Align::Center);
    std::vector<std::string> items = {E.final ? "Roll Credits" : "Continue", "Return to Title"};
    for (int i = 0; i < 2; i++) {
        Vec2 p{cx, 860 + i * 56.f};
        if (m_sel == i) r.textured(r.brushTex(), true, {p.x - 200, p.y - 36, 400, 50}, {0, 0, 1, 1}, col::crimson.withA(0.9f * ba));
        T.draw(r, Font::Bold, 26, p, items[i], (m_sel == i ? col::paper : col::paperDim).withA(ba), Align::Center);
    }
}

void Game::drawChapters() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.9f));
    drawHeader("章", "Chapters", "Replay any chapter you have reached");
    float W = r.uiWidth();
    auto& defs = chapterDefs();
    for (size_t i = 0; i < defs.size(); i++) {
        const ChapterDef& c = defs[i];
        bool open = c.number <= m_profile.chaptersCleared + 1;
        bool on = (int)i == m_sel;
        Vec2 p{180, 290 + i * 104.f};
        if (on) r.textured(r.brushTex(), true, {p.x - 60, p.y - 52, W - 240, 92}, {0, 0, 1, 1}, col::crimson.withA(0.75f));
        T.draw(r, Font::Kanji, 34, p + Vec2{0, -2}, c.numeral, (open ? col::paper : col::muted).withA(on ? 1.f : 0.8f));
        T.draw(r, on ? Font::Bold : Font::Body, 28, p + Vec2{190, -8}, open ? c.title : "Sealed", open ? col::paper : col::muted);
        if (open) {
            std::string rec = "Not yet cleared";
            auto it = m_profile.chapterBest.find(c.level);
            if (it != m_profile.chapterBest.end()) {
                float tm = m_profile.chapterTime.count(c.level) ? m_profile.chapterTime.at(c.level) : 0;
                rec = strfmt("Best %d  (%s)   ·   Fastest %d:%02d", it->second, m_profile.chapterGrade.count(c.level) ? m_profile.chapterGrade.at(c.level).c_str() : "-", (int)(tm / 60), (int)tm % 60);
            }
            T.draw(r, Font::Body, 19, p + Vec2{190, 24}, rec, col::paperDim);
        }
        T.draw(r, Font::Kanji, 30, {W - 180, p.y}, c.kanji, (open ? col::paperDim : col::muted).withA(0.6f), Align::Right);
    }
    drawRich(r, *m_in, Font::Body, 18, {W / 2, 1040}, "{Confirm} play     {Cancel} back", col::paperDim, Align::Center);
}

void Game::drawHowTo() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.93f));
    drawHeader("道", "How to Play", m_col == 0 ? "Controls  ·  page 1 / 2   [←/→]" : "Techniques  ·  page 2 / 2   [←/→]");
    float W = r.uiWidth();
    if (m_col == 0) {
        // Designed for a controller first; the keyboard column is the fallback mapping.
        struct Row { const char* action; int act; };
        const Row rows[] = {
            {"Move  ·  a light tilt walks silently", -1}, {"Jump  (hold for height)", (int)Act::Jump},
            {"Strike  (chain 3–4 cuts)", (int)Act::Attack}, {"Guard  ·  tap to Deflect", (int)Act::Guard},
            {"Veil-Step  (untouchable dash)", (int)Act::Dash}, {"Veil-Thread  (hold near a ring)", (int)Act::Grapple},
            {"Throw shuriken", (int)Act::Shuriken}, {"Aim throws & threads  ·  peek camera", -2},
            {"Ember Art  ·  Interact when prompted", (int)Act::Art}, {"Drink from gourd", (int)Act::Heal},
            {"Crouch  ·  drop through beams", (int)Act::Down}, {"Equipment", (int)Act::Menu}, {"Pause  ·  hold to skip scenes", (int)Act::Pause},
        };
        Input::Style pad = m_in->padStyle();
        float x0 = std::max(110.f, W / 2 - 760), y = 250;
        T.draw(r, Font::Bold, 20, {x0, y}, "ACTION", col::gold, Align::Left, 3);
        T.draw(r, Font::Bold, 20, {x0 + 640, y}, pad == Input::Style::Xbox ? "CONTROLLER (XBOX)" : "CONTROLLER (PLAYSTATION)", col::gold, Align::Left, 3);
        T.draw(r, Font::Bold, 20, {x0 + 1080, y}, "KEYBOARD (FALLBACK)", col::gold, Align::Left, 3);
        y += 22;
        r.rect({x0, y, 1480, 1.5f}, col::paperDim.withA(0.3f));
        y += 44;
        for (auto& row : rows) {
            T.draw(r, Font::Body, 22, {x0, y}, row.action, col::paper);
            Vec2 gc{x0 + 680, y - 8};
            if (row.act >= 0) {
                drawGlyph(r, *m_in, (Act)row.act, gc, 34, 1, pad);
                if (row.act == (int)Act::Art) drawGlyph(r, *m_in, Act::Up, gc + Vec2{52, 0}, 30, 1, pad);
                drawGlyph(r, *m_in, (Act)row.act, {x0 + 1110, y - 8}, 34, 1, Input::Style::Keyboard);
            } else {
                drawRich(r, *m_in, Font::Body, 20, {x0 + 662, y}, row.act == -1 ? "Left stick / D-pad" : "Right stick", col::paperDim);
                T.draw(r, Font::Bold, 20, {x0 + 1092, y}, row.act == -1 ? "A / D   (S crouch)" : "W / S lean the throw", col::paperDim);
            }
            y += 46;
        }
        T.draw(r, Font::Italic, 19, {x0, y + 12}, m_in->hasPad() ? "Controller connected: light bar and haptics reflect Rei's state." : "Connect a PS4/PS5 or Xbox controller for the intended experience.", col::paperDim);
    } else {
        struct Tip { const char* k; const char* title; const char* text; };
        const Tip tips[] = {
            {"流", "Deflect", "Tap Guard the instant a blow lands. No damage, heavy posture damage to the attacker, arrows fly back. Holding Guard only blocks and still hurts."},
            {"破", "Posture & Execution", "Strikes and deflections fill a foe's posture bar. When it breaks, a vermilion mark appears: Strike to Execute. Bosses need one Execution per pip."},
            {"危", "Perilous attacks", "The red 危 diamond means the attack cannot be guarded. Veil-Step through it, or jump a sweep."},
            {"忍", "Stealth", "Foes see in front of them; a ring fills above their head as they notice you. Crouch, hide in tall grass, and strike from behind or above to Assassinate."},
            {"糸", "Veil-Step & Veil-Thread", "Dashing makes you untouchable. Hold Grapple near a glowing ring to swing; release to fly. Wall-slide and jump to climb shafts."},
            {"風", "Spark, Ember & Stances", "Hits and deflections build Sparks for the Ember Art. Win the trial at a wind beacon to awaken the Gale Stance and its skill branch."},
            {"影", "Kage Rank", "Varied, flawless play raises your style rank from D to SS. Getting hit drops it. Your chapter score and grade are recorded as personal bests."},
            {"火", "Camps", "Rest to heal, refill gourds and save. Resting returns your foes. Rest to learn skills; forge upgrades from materials."},
        };
        float x0 = std::max(110.f, W / 2 - 800), y = 260;
        for (int i = 0; i < 8; i++) {
            float cx = x0 + (i % 2) * 820, cy = y + (i / 2) * 190;
            drawSeal({cx + 30, cy + 10}, 52, tips[i].k, i == 2 ? col::crimson : rgb(0x3a3a48));
            T.draw(r, Font::Bold, 25, {cx + 74, cy + 18}, tips[i].title, col::paper);
            auto lines = T.wrap(Font::Body, 19, tips[i].text, 700);
            for (size_t k = 0; k < lines.size() && k < 4; k++) T.draw(r, Font::Body, 19, {cx + 74, cy + 52 + k * 27.f}, lines[k], col::paperDim);
        }
    }
    drawRich(r, *m_in, Font::Body, 18, {130, 1050}, "{Left}{Right} page     {Cancel} back", col::paperDim);
}

void Game::drawCredits() {
    Renderer& r = *m_r;
    TextSystem& T = r.text();
    r.rect({0, 0, r.uiWidth(), 1080}, col::ink.withA(0.94f));
    float cx = r.uiWidth() / 2;
    const char* lines[] = {
        "SHADOW VEIL", "",
        "Rei  ·  Odaka Sen  ·  Sayo  ·  Hana  ·  Fuki", "Tetsu  ·  Funaki  ·  Karasu  ·  Lord Hakuro  ·  Kagami", "",
        "Design, code, art direction, voice score & sound", "All drawn and synthesised at runtime — no samples, no instruments", "",
        "Built with", "SDL3 · OpenGL · FreeType · zlib · C++20", "",
        "Typography", "Noto Serif & Noto Serif CJK (SIL Open Font License)", "",
        "Thank you for playing.",
    };
    // a slow scroll after the finale, a still page from the title
    bool roll = m_prevScreen == Screen::ChapterEnd;
    float y = roll ? 1100 - std::min(m_screenT * 40.f, 880.f) : 200;
    for (auto* l : lines) {
        bool head = std::string(l) == "SHADOW VEIL";
        T.draw(r, head ? Font::Bold : Font::Body, head ? 48 : 24, {cx, y}, l, head ? col::paper : col::paperDim, Align::Center, head ? 12 : 1);
        y += head ? 60 : 38;
    }
    drawSeal({cx, y + 50}, 64, "影", col::crimson);
}

} // namespace sv
