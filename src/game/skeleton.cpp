#include "game/skeleton.h"
#include "render/renderer.h"

namespace sv {

Pose lerp(const Pose& a, const Pose& b, float t) {
    Pose o;
    o.hipX = lerp(a.hipX, b.hipX, t); o.hipY = lerp(a.hipY, b.hipY, t);
    o.lean = lerp(a.lean, b.lean, t); o.head = lerp(a.head, b.head, t);
    o.armFU = lerp(a.armFU, b.armFU, t); o.armFL = lerp(a.armFL, b.armFL, t);
    o.armBU = lerp(a.armBU, b.armBU, t); o.armBL = lerp(a.armBL, b.armBL, t);
    o.legFU = lerp(a.legFU, b.legFU, t); o.legFL = lerp(a.legFL, b.legFL, t);
    o.legBU = lerp(a.legBU, b.legBU, t); o.legBL = lerp(a.legBL, b.legBL, t);
    o.sword = lerpAngle(a.sword, b.sword, t);
    o.swordLen = lerp(a.swordLen, b.swordLen, t);
    return o;
}

Pose P(float hx, float hy, float lean, float head, float afu, float afl, float abu, float abl,
       float lfu, float lfl, float lbu, float lbl, float sword) {
    Pose p;
    p.hipX = hx; p.hipY = hy;
    p.lean = lean * DEG; p.head = head * DEG;
    p.armFU = afu * DEG; p.armFL = afl * DEG; p.armBU = abu * DEG; p.armBL = abl * DEG;
    p.legFU = lfu * DEG; p.legFL = lfl * DEG; p.legBU = lbu * DEG; p.legBL = lbl * DEG;
    p.sword = sword * DEG;
    return p;
}

Joints solve(const Rig& rig, const Pose& p, Vec2 root, float facing) {
    Joints j;
    j.facing = facing;
    const float s = rig.scale;
    auto W = [&](Vec2 local) { return Vec2{root.x + local.x * facing * s, root.y + local.y * s}; };
    Vec2 hip{p.hipX, -(rig.thigh + rig.shin) + p.hipY};
    // spine: positive lean tilts the top forward
    Vec2 spineDir{std::sin(p.lean), -std::cos(p.lean)};
    Vec2 chest = hip + spineDir * rig.spine;
    Vec2 neck = chest + spineDir * rig.neck;
    float headA = p.lean + p.head;
    Vec2 head = neck + Vec2{std::sin(headA), -std::cos(headA)} * rig.headR;
    float down = -p.lean;  // "down along the torso" in limb-angle space
    Vec2 shoulder = chest - spineDir * 3.0f;
    Vec2 shF = shoulder + Vec2{rig.shoulderW * 0.5f, 0}, shB = shoulder - Vec2{rig.shoulderW * 0.5f, 0};
    float aF = down + p.armFU, aFl = aF + p.armFL;
    float aB = down + p.armBU, aBl = aB + p.armBL;
    Vec2 elF = shF + dirFromAngle(aF) * rig.upperArm, haF = elF + dirFromAngle(aFl) * rig.foreArm;
    Vec2 elB = shB + dirFromAngle(aB) * rig.upperArm, haB = elB + dirFromAngle(aBl) * rig.foreArm;
    float lF = p.legFU, lFl = lF + p.legFL, lB = p.legBU, lBl = lB + p.legBL;
    Vec2 knF = hip + dirFromAngle(lF) * rig.thigh, ftF = knF + dirFromAngle(lFl) * rig.shin;
    Vec2 knB = hip + dirFromAngle(lB) * rig.thigh, ftB = knB + dirFromAngle(lBl) * rig.shin;
    float sw = aFl + p.sword;
    Vec2 swDir = dirFromAngle(sw);
    Vec2 swBase = haF - swDir * (rig.handleLen * 0.5f);
    Vec2 swTip = haF + swDir * (rig.bladeLen * p.swordLen);

    j.root = root; j.hip = W(hip); j.chest = W(chest); j.neck = W(neck); j.head = W(head);
    j.shoulderF = W(shF); j.elbowF = W(elF); j.handF = W(haF);
    j.shoulderB = W(shB); j.elbowB = W(elB); j.handB = W(haB);
    j.kneeF = W(knF); j.footF = W(ftF); j.kneeB = W(knB); j.footB = W(ftB);
    j.swordBase = W(swBase); j.swordTip = W(swTip);
    // world angles (mirrored for facing): angle a in local space -> -a when facing left
    j.headAngle = headA * facing; j.swordAngle = sw * facing; j.spineAngle = p.lean * facing; j.foreArmFAngle = aFl * facing;
    return j;
}

void Cloth::init(Vec2 anchor, int n, float seg) {
    pts.assign(n, anchor); prev.assign(n, anchor); segLen = seg;
    for (int i = 0; i < n; i++) { pts[i] = anchor + Vec2{-seg * i, 0}; prev[i] = pts[i]; }
}

void Cloth::update(Vec2 anchor, Vec2 gravity, Vec2 wind, float dt, float damping, float floorY) {
    if (pts.empty()) return;
    pts[0] = anchor; prev[0] = anchor;
    for (size_t i = 1; i < pts.size(); i++) {
        Vec2 v = (pts[i] - prev[i]) * damping;
        prev[i] = pts[i];
        float flutter = 1.0f + 0.6f * std::sin(i * 0.9f + pts[i].x * 0.02f);
        pts[i] += v + (gravity + wind * flutter) * (dt * dt);
    }
    for (int it = 0; it < 4; it++) {
        pts[0] = anchor;
        for (size_t i = 1; i < pts.size(); i++) {
            Vec2 d = pts[i] - pts[i - 1];
            float L = d.len();
            if (L < 1e-4f) continue;
            float diff = (L - segLen) / L;
            if (i == 1) pts[i] -= d * diff;
            else { pts[i] -= d * (diff * 0.5f); pts[i - 1] += d * (diff * 0.5f); }
        }
        // cloth rests on the ground instead of sinking through it
        for (size_t i = 1; i < pts.size(); i++)
            if (pts[i].y > floorY) { pts[i].y = floorY; prev[i].y = lerp(prev[i].y, floorY, 0.5f); prev[i].x = lerp(prev[i].x, pts[i].x, 0.3f); }
    }
}

void drawClothStrip(Renderer& r, const Cloth& c, float w0, float w1, Color col, Color rim, Vec2 lightDir, float rimW) {
    size_t n = c.pts.size();
    if (n < 2) return;
    auto edge = [&](size_t i, float& w, Vec2& nrm) {
        float t = i / (float)(n - 1);
        w = lerp(w0, w1, t);
        Vec2 d = (i + 1 < n ? c.pts[i + 1] - c.pts[i] : c.pts[i] - c.pts[i - 1]).norm();
        nrm = d.perp();
    };
    for (int pass = 0; pass < 2; pass++) {
        Vec2 off = pass == 0 ? lightDir * rimW : Vec2{0, 0};
        Color k = pass == 0 ? rim : col;
        for (size_t i = 0; i + 1 < n; i++) {
            float wa, wb; Vec2 na, nb;
            edge(i, wa, na); edge(i + 1, wb, nb);
            Vec2 a = c.pts[i] + off, b = c.pts[i + 1] + off;
            r.quad(a + na * (wa * .5f), b + nb * (wb * .5f), b - nb * (wb * .5f), a - na * (wa * .5f), k);
        }
    }
}

void drawHumanoid(Renderer& r, const Rig& rig, const Joints& j, const RenderStyle& st,
                  const std::function<void(GearLayer, bool)>& gear, bool drawSword) {
    const float s = rig.scale;
    Color body = st.body.withA(st.alpha), back = st.bodyBack.withA(st.alpha), rim = st.rim.mulA(st.alpha);
    Vec2 L = st.lightDir.norm() * (st.rimWidth * s);

    auto part = [&](auto&& fn, Color c) {
        fn(L, rim);
        fn(Vec2{0, 0}, c);
    };
    auto limb = [&](Vec2 a, Vec2 b, Vec2 c, float r0, float r1, float r2, Color col) {
        part([&](Vec2 o, Color k) {
            r.capsule(a + o, b + o, r0 * s, r1 * s, k);
            r.capsule(b + o, c + o, r1 * s, r2 * s, k);
        }, col);
    };

    if (gear) { gear(GearLayer::Back, true); gear(GearLayer::Back, false); }
    // back limbs
    limb(j.hip, j.kneeB, j.footB, rig.limbR * 1.25f, rig.limbR, rig.limbR2, back);
    limb(j.shoulderB, j.elbowB, j.handB, rig.limbR, rig.limbR2 * 1.05f, rig.limbR2, back);
    // torso: tapered from pelvis to chest, then neck and head
    part([&](Vec2 o, Color k) {
        r.capsule(j.hip + o, j.chest + o, rig.hipR * s, rig.chestR * s, k);
        r.capsule(j.chest + o, j.neck + o, rig.chestR * 0.6f * s, rig.limbR2 * s, k);
        if (gear) gear(GearLayer::Torso, o.x != 0 || o.y != 0);
        r.circle(j.head + o, rig.headR * s, k);
        if (gear) gear(GearLayer::Head, o.x != 0 || o.y != 0);
    }, body);
    // front leg
    limb(j.hip, j.kneeF, j.footF, rig.limbR * 1.25f, rig.limbR, rig.limbR2, body);
    // feet as small wedges
    for (int f = 0; f < 2; f++) {
        Vec2 ft = f ? j.footF : j.footB;
        Color c = f ? body : back;
        r.capsule(ft + Vec2{-3 * s * j.facing, 0}, ft + Vec2{6 * s * j.facing, 1 * s}, 2.6f * s, 2.0f * s, c);
    }
    if (gear) { gear(GearLayer::Front, true); gear(GearLayer::Front, false); }
    // sword (behind the front arm's hand, in front of the body)
    if (drawSword) {
        Vec2 d = (j.swordTip - j.swordBase).norm();
        Vec2 handleEnd = j.handF - d * (rig.handleLen * s * 0.6f);
        Vec2 guard = j.handF + d * (3 * s);
        Vec2 n = d.perp();
        // blade: slight curve via two segments
        Vec2 mid = lerp(guard, j.swordTip, 0.55f) + n * (1.5f * s * j.facing);
        Color bladeC = st.blade.mulA(st.alpha);
        r.lineGrad(guard, mid, 2.6f * s, 2.3f * s, bladeC * 0.55f, bladeC * 0.7f);
        r.lineGrad(mid, j.swordTip, 2.3f * s, 0.6f * s, bladeC * 0.7f, bladeC * 0.8f);
        // bright edge
        Color edge = Color{1, 1, 1, 0.55f * st.alpha};
        r.line(guard + n * (0.9f * s), mid + n * (0.9f * s), 0.7f * s, edge);
        r.line(mid + n * (0.9f * s), j.swordTip, 0.6f * s, edge);
        r.line(handleEnd, guard, 3.4f * s, body);
        r.capsule(guard - n * (3.5f * s), guard + n * (3.5f * s), 1.6f * s, 1.6f * s, Color{0.18f, 0.15f, 0.08f, st.alpha});
        if (gear) gear(GearLayer::Weapon, false);
    } else if (gear) gear(GearLayer::Weapon, false);
    // front arm last so it reads over the torso
    limb(j.shoulderF, j.elbowF, j.handF, rig.limbR, rig.limbR2 * 1.05f, rig.limbR2, body);
}

} // namespace sv
