#pragma once
#include "core/math.h"
#include <vector>
#include <functional>

namespace sv {

class Renderer;

// Joint angles in radians. Limb angles are measured from straight down; positive swings forward
// (toward the facing direction). Knee/elbow angles are relative to their parent bone.
struct Pose {
    float hipX = 0, hipY = 0;   // pelvis offset from the standing position (y+ = down)
    float lean = 0;             // torso lean, positive = forward
    float head = 0;             // head tilt, positive = chin down / forward
    float armFU = 0, armFL = 0, armBU = 0, armBL = 0;
    float legFU = 0, legFL = 0, legBU = 0, legBL = 0;
    float sword = 0;            // blade relative to front forearm
    float swordLen = 1;         // blade visibility (sheathing)
};
Pose lerp(const Pose& a, const Pose& b, float t);
// Authoring helper in degrees: (hipX, hipY, lean, head, aFU, aFL, aBU, aBL, lFU, lFL, lBU, lBL, sword)
Pose P(float hx, float hy, float lean, float head, float afu, float afl, float abu, float abl,
       float lfu, float lfl, float lbu, float lbl, float sword);

struct Rig {
    float scale = 1.0f;
    float thigh = 20, shin = 20, spine = 25, neck = 5, headR = 8.5f;
    float upperArm = 15, foreArm = 14;
    float hipR = 7.5f, chestR = 10.5f, limbR = 4.2f, limbR2 = 3.0f;
    float bladeLen = 54, handleLen = 11;
    float shoulderW = 0;   // broad-shouldered characters
};

// World-space joint positions after forward kinematics.
struct Joints {
    Vec2 root, hip, chest, neck, head;
    Vec2 shoulderF, elbowF, handF, shoulderB, elbowB, handB;
    Vec2 kneeF, footF, kneeB, footB;
    Vec2 swordBase, swordTip;
    float headAngle = 0, swordAngle = 0, spineAngle = 0, foreArmFAngle = 0;
    float facing = 1;
};

Joints solve(const Rig& rig, const Pose& p, Vec2 root, float facing);

// Verlet rope for scarves, headband tails, banners, ropes.
struct Cloth {
    std::vector<Vec2> pts, prev;
    float segLen = 6;
    void init(Vec2 anchor, int n, float seg);
    void update(Vec2 anchor, Vec2 gravity, Vec2 wind, float dt, float damping = 0.97f, float floorY = 1e9f);
};

struct RenderStyle {
    Color body{0.035f, 0.035f, 0.05f};
    Color bodyBack{0.02f, 0.02f, 0.03f};
    Color rim{0.55f, 0.65f, 0.95f};
    Vec2 lightDir{0.6f, -0.8f};    // direction TOWARD the light
    float rimWidth = 1.8f;
    Color blade{0.75f, 0.78f, 0.85f};
    Color accent{0.70f, 0.07f, 0.12f};
    float alpha = 1.0f;
};

// Draws a humanoid. `gear` is invoked between layers so callers can add hats, armour, weapons.
enum class GearLayer { Back, Torso, Head, Front, Weapon };
void drawHumanoid(Renderer& r, const Rig& rig, const Joints& j, const RenderStyle& st,
                  const std::function<void(GearLayer, bool rimPass)>& gear = nullptr, bool drawSword = true);
void drawClothStrip(Renderer& r, const Cloth& c, float w0, float w1, Color col, Color rim, Vec2 lightDir, float rimW);

} // namespace sv
