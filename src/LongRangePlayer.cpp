#include "LongRangePlayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

using glm::vec2;
using glm::vec4;
using glm::mat4;

namespace {
// ---- Palette: Egyptian archer (sun-tanned skin, linen, leather, gold, turquoise) ----
const vec4 SKIN       (0.76f, 0.54f, 0.38f, 1.0f);
const vec4 SKIN_LIGHT (0.84f, 0.62f, 0.45f, 1.0f);
const vec4 SKIN_DARK  (0.60f, 0.40f, 0.28f, 1.0f);
const vec4 HAIR       (0.10f, 0.07f, 0.06f, 1.0f);
const vec4 HAIR_LIGHT (0.24f, 0.17f, 0.14f, 1.0f);
const vec4 LINEN      (0.91f, 0.86f, 0.72f, 1.0f);
const vec4 LINEN_DARK (0.74f, 0.67f, 0.52f, 1.0f);
const vec4 TROUSER    (0.50f, 0.40f, 0.27f, 1.0f);
const vec4 TROUSER_DK (0.36f, 0.28f, 0.19f, 1.0f);
const vec4 LEATHER    (0.42f, 0.26f, 0.14f, 1.0f);
const vec4 LEATHER_DK (0.25f, 0.15f, 0.09f, 1.0f);
const vec4 GOLD       (0.95f, 0.76f, 0.22f, 1.0f);
const vec4 GOLD_DARK  (0.72f, 0.52f, 0.12f, 1.0f);
const vec4 TURQUOISE  (0.15f, 0.68f, 0.64f, 1.0f);
const vec4 LAPIS      (0.12f, 0.25f, 0.62f, 1.0f);
const vec4 WOOD       (0.55f, 0.36f, 0.18f, 1.0f);
const vec4 WOOD_DARK  (0.34f, 0.20f, 0.10f, 1.0f);
const vec4 STRING_COL (0.93f, 0.91f, 0.82f, 1.0f);
const vec4 BRONZE     (0.74f, 0.50f, 0.24f, 1.0f);
const vec4 FEATHER_W  (0.96f, 0.95f, 0.90f, 1.0f);
const vec4 BLACK      (0.05f, 0.04f, 0.04f, 1.0f);
const vec4 LIPS       (0.62f, 0.28f, 0.24f, 1.0f);

const float HIT_TIME      = 0.35f;   // shake / flash after being hit
const float JUMP_TIME     = 0.70f;
const float DUCK_TIME     = 0.70f;
const float JUMP_HEIGHT   = 0.38f;   // model units
const float DUCK_DROP     = 0.15f;   // how far the body sinks when ducking
const float CYCLE         = 1.30f;   // seconds between two arrows
const float DRAW_START    = 0.15f;   // nocks an arrow and starts pulling
const float RELEASE       = 0.85f;   // lets go
const float IMPACT_TIME   = 0.30f;   // arrow hit animation length

const float GRIP_X = 0.34f, HAND_Y = 0.765f;   // bow hand (upper-body frame)
const float BOW_HALF = 0.30f, BOW_BELLY = 0.075f;
const float TIP_X = GRIP_X - BOW_BELLY;        // string anchor x (relaxed)
const float PULL = 0.20f;                      // how far the string is pulled back
const float ARROW_HALF = 0.275f;               // half arrow length

mat4 at(const mat4& m, float x, float y, float deg = 0.0f)
{
    mat4 r = glm::translate(m, glm::vec3(x, y, 0.0f));
    if (deg != 0.0f) r = glm::rotate(r, glm::radians(deg), glm::vec3(0, 0, 1));
    return r;
}

float smooth(float x) { x = std::min(1.0f, std::max(0.0f, x)); return x * x * (3.0f - 2.0f * x); }

// Two-bone IK: returns the middle joint (knee / elbow). bend = +1 / -1 picks the side.
vec2 ik(vec2 a, vec2 b, float l1, float l2, float bend)
{
    vec2 d = b - a;
    float dist = glm::length(d);
    if (dist < 1e-4f) return a;
    dist = std::min(dist, l1 + l2 - 1e-3f);
    vec2 dir = glm::normalize(d);
    float x = (l1 * l1 - l2 * l2 + dist * dist) / (2.0f * dist);
    float h = std::sqrt(std::max(0.0f, l1 * l1 - x * x));
    return a + dir * x + vec2(-dir.y, dir.x) * h * bend;
}

// small rotated rectangle across a limb a->b (wraps, bands, straps)
void band(Renderer2D& r, const mat4& m, vec2 a, vec2 b, float t, float across, float thick,
          const vec4& col, float red)
{
    vec2 d = glm::normalize(b - a), n(-d.y, d.x), p = a + (b - a) * t;
    vec2 u = d * thick * 0.5f, v = n * across * 0.5f;
    r.drawConvex({p - u - v, p + u - v, p + u + v, p - u + v}, col, m, red);
}

vec4 alpha(vec4 c, float a) { c.a = a; return c; }

// An arrow centred on 'c' (model units, facing +x). 'a' = opacity.
void arrowShape(Renderer2D& r, const mat4& m, vec2 c, float a)
{
    r.drawLimb(c + vec2(-ARROW_HALF, 0.0f), c + vec2(0.22f, 0.0f), 0.012f, 0.012f, alpha(WOOD, a), m);
    // bronze head
    r.drawConvex({c + vec2(ARROW_HALF, 0.0f), c + vec2(0.205f, 0.026f),
                  c + vec2(0.185f, 0.0f), c + vec2(0.205f, -0.026f)}, alpha(BRONZE, a), m);
    // fletching (turquoise + white feathers) and nock
    r.drawConvex({c + vec2(-0.275f, 0.0f), c + vec2(-0.195f, 0.0f),
                  c + vec2(-0.215f, 0.034f), c + vec2(-0.285f, 0.034f)}, alpha(TURQUOISE, a), m);
    r.drawConvex({c + vec2(-0.275f, 0.0f), c + vec2(-0.195f, 0.0f),
                  c + vec2(-0.215f, -0.034f), c + vec2(-0.285f, -0.034f)}, alpha(FEATHER_W, a), m);
    r.drawRect(c + vec2(-0.285f, 0.0f), vec2(0.014f, 0.016f), alpha(GOLD_DARK, a), m);
}
} // namespace

LongRangePlayer::LongRangePlayer(vec2 feetPos, float height)
    : pos_(feetPos), height_(height), speed_(0.40f * height), arrowSpeed_(2.6f * height) {}

float LongRangePlayer::jumpOffset() const
{
    if (jumpT_ <= 0.0f) return 0.0f;
    float u = 1.0f - jumpT_ / JUMP_TIME;
    return JUMP_HEIGHT * 4.0f * u * (1.0f - u);
}

float LongRangePlayer::duckAmount() const
{
    if (duckT_ <= 0.0f) return 0.0f;
    float u = 1.0f - duckT_ / DUCK_TIME;
    return smooth(std::min(u, 1.0f - u) / 0.15f);
}

float LongRangePlayer::drawAmount() const
{
    if (cycle_ < DRAW_START || cycle_ >= RELEASE) return 0.0f;
    return smooth((cycle_ - DRAW_START) / (RELEASE - DRAW_START));
}

float LongRangePlayer::vibration() const
{
    if (cycle_ < RELEASE) return 0.0f;
    float t = cycle_ - RELEASE;
    return std::exp(-t * 18.0f) * std::sin(t * 90.0f);
}

void LongRangePlayer::fire()
{
    float nockY = HAND_Y - DUCK_DROP * duckAmount() + jumpOffset();
    Arrow a;
    a.pos = vec2(pos_.x + (TIP_X - PULL + ARROW_HALF) * height_, pos_.y + nockY * height_);
    a.previous = a.pos;
    arrows_.push_back(a);
}

void LongRangePlayer::arrowHit(size_t i)
{
    if (i < arrows_.size() && !arrows_[i].hit) { arrows_[i].hit = true; arrows_[i].hitTime = 0.0f; }
}

bool LongRangePlayer::arrowHitsEnemy(size_t i, float enemyXMin, float enemyXMax)
{
    if (i >= arrows_.size() || arrows_[i].hit || enemyXMin > enemyXMax) return false;
    const Arrow& arrow = arrows_[i];
    const float halfLength = ARROW_HALF * height_;
    const float sweptMin = std::min(arrow.previous.x, arrow.pos.x) - halfLength;
    const float sweptMax = std::max(arrow.previous.x, arrow.pos.x) + halfLength;
    if (sweptMax < enemyXMin || sweptMin > enemyXMax) return false;
    arrowHit(i);
    return true;
}

void LongRangePlayer::jump()
{
    if (hp_ > 0 && jumpT_ <= 0.0f && duckT_ <= 0.0f) jumpT_ = JUMP_TIME;
}

void LongRangePlayer::duck()
{
    if (hp_ > 0 && jumpT_ <= 0.0f && duckT_ <= 0.0f) duckT_ = DUCK_TIME;
}

void LongRangePlayer::takeHit(int damage)
{
    if (hp_ <= 0 || damage <= 0) return;
    hp_ = damage >= hp_ ? 0 : hp_ - damage;
    hitTimer_ = HIT_TIME;
}

void LongRangePlayer::reset()
{
    hp_ = MAX_HEALTH;
    hitTimer_ = 0.0f;
    cycle_ = 0.35f;
    fired_ = false;
    jumpT_ = duckT_ = 0.0f;
    arrows_.clear();
}

void LongRangePlayer::update(float dt)
{
    time_ += dt;
    if (hitTimer_ > 0.0f) hitTimer_ = std::max(0.0f, hitTimer_ - dt);

    if (hp_ > 0) {
        // bow cycle: nock -> draw -> release -> rest, forever (also while walking)
        cycle_ += dt;
        if (!fired_ && cycle_ >= RELEASE) { fire(); fired_ = true; }
        if (cycle_ >= CYCLE) { cycle_ -= CYCLE; fired_ = false; }

        if (jumpT_ > 0.0f) jumpT_ = std::max(0.0f, jumpT_ - dt);
        if (duckT_ > 0.0f) duckT_ = std::max(0.0f, duckT_ - dt);

        bool moving = !engaged_;
        walkBlend_ += ((moving ? 1.0f : 0.0f) - walkBlend_) * std::min(1.0f, dt * 8.0f);
        if (moving) { pos_.x += speed_ * dt; phase_ += dt * 7.0f; }
    }

    // arrows keep flying even if she has been defeated
    for (Arrow& a : arrows_) {
        a.previous = a.pos;
        if (a.hit) a.hitTime += dt;
        else       a.pos.x += arrowSpeed_ * dt;
    }
    arrows_.erase(std::remove_if(arrows_.begin(), arrows_.end(), [&](const Arrow& a) {
        return (a.hit && a.hitTime >= IMPACT_TIME) || a.pos.x - ARROW_HALF * height_ > arrowMaxX_;
    }), arrows_.end());
}

// ------------------------------------------------------------------
// Hierarchy:  base (world position + scale)
//               root  (+ hit shake, jump)     legs
//                 upper (+ breathing, duck)   quiver, torso, head, arms, bow
// ------------------------------------------------------------------
void LongRangePlayer::draw(Renderer& renderer) const
{
    Renderer2D r(renderer);
    if (hp_ <= 0) return;   // defeated -> removed from the scene

    float flash = hitTimer_ / HIT_TIME;
    float shake = std::sin(hitTimer_ * 70.0f) * 0.02f * flash;
    float bob   = std::sin(time_ * 2.4f) * 0.005f + walkBlend_ * std::fabs(std::sin(phase_)) * 0.012f;
    float lost  = 1.0f - (float)hp_ / (float)MAX_HEALTH;
    float red   = std::min(1.0f, lost * 0.75f + flash * 0.25f);

    float ja   = jumpOffset();
    float da   = duckAmount();
    float drop = DUCK_DROP * da;
    float draw = drawAmount();
    float vib  = vibration();

    mat4 base  = glm::scale(glm::translate(mat4(1.0f), glm::vec3(pos_, 0.0f)),
                            glm::vec3(height_, height_, 1.0f));
    mat4 root  = at(base, shake, ja);
    mat4 upper = at(root, 0.0f, bob - drop);

    // ----------------------------------------------------------- legs
    float jam = std::min(1.0f, ja / JUMP_HEIGHT);   // 0..1 how high in the jump (tucks legs)
    for (int i = 0; i < 2; ++i) {
        float s = std::sin(phase_ + i * 3.14159f), c = std::cos(phase_ + i * 3.14159f);
        vec2 stance = i == 0 ? vec2(-0.09f, 0.06f) : vec2(0.11f, 0.06f);
        vec2 walk(-0.02f + (i ? 0.04f : 0.0f) + 0.11f * s, 0.06f + 0.05f * std::max(0.0f, c));
        vec2 ank = glm::mix(stance, walk, walkBlend_);
        ank.y += 0.14f * jam;
        ank.x += (i ? 0.05f : -0.03f) * jam;

        vec2 hip((i ? 0.05f : -0.05f), 0.50f - drop);
        vec2 knee = ik(hip, ank, 0.23f, 0.23f, 1.0f);

        // boot (behind the trouser so the cuff overlaps it)
        r.drawConvex({ank + vec2(-0.05f, -0.045f), ank + vec2(0.115f, -0.045f), ank + vec2(0.125f, 0.005f),
                      ank + vec2(0.06f, 0.05f), ank + vec2(-0.05f, 0.07f)}, LEATHER_DK, root, red);
        r.drawRect(ank + vec2(0.035f, -0.040f), vec2(0.175f, 0.012f), BLACK, root, red);   // sole
        r.drawLimb(ank + vec2(-0.005f, 0.05f), ank + vec2(0.0f, 0.12f), 0.085f, 0.095f, LEATHER, root, red);
        for (int k = 0; k < 3; ++k)                                                          // laces
            r.drawEllipse(ank + vec2(0.012f, 0.062f + k * 0.024f), vec2(0.008f), GOLD_DARK, root, red, 10);

        // trousers with linen leg wraps
        r.drawLimb(hip, knee, 0.125f, 0.10f, TROUSER, root, red);
        r.drawLimb(knee, ank + vec2(0.0f, 0.10f), 0.10f, 0.088f, TROUSER, root, red);
        r.drawLimb(hip + vec2(-0.012f, -0.02f), knee + vec2(-0.012f, 0.02f), 0.03f, 0.03f, TROUSER_DK, root, red);
        for (int k = 0; k < 4; ++k)
            band(r, root, knee, ank + vec2(0, 0.10f), 0.22f + k * 0.2f, 0.10f, 0.026f, LINEN_DARK, red);
        r.drawEllipse(knee + vec2(0.012f, 0.0f), vec2(0.04f, 0.035f), LEATHER, root, red, 14);   // knee pad
        r.drawEllipse(knee + vec2(0.012f, 0.0f), vec2(0.016f), GOLD, root, red, 10);
    }

    // --------------------------------------------------- quiver (behind her back)
    vec2 qa(-0.10f, 0.56f), qb(-0.19f, 0.87f);
    for (int k = 0; k < 3; ++k) {   // arrows standing in the quiver
        float dx = (k - 1) * 0.028f;
        vec2 a0 = qb + vec2(dx * 0.4f, -0.04f), a1 = qb + vec2(-0.015f + dx * 1.7f, 0.14f - 0.02f * (k == 1));
        r.drawLimb(a0, a1, 0.012f, 0.012f, WOOD, upper, red);
        vec2 d = glm::normalize(a1 - a0), n(-d.y, d.x);
        r.drawConvex({a1, a1 - d * 0.075f + n * 0.03f, a1 - d * 0.11f + n * 0.03f, a1 - d * 0.035f},
                     k == 1 ? FEATHER_W : TURQUOISE, upper, red);
        r.drawConvex({a1, a1 - d * 0.075f - n * 0.03f, a1 - d * 0.11f - n * 0.03f, a1 - d * 0.035f},
                     k == 1 ? TURQUOISE : FEATHER_W, upper, red);
    }
    r.drawLimb(qa, qb, 0.105f, 0.095f, LEATHER, upper, red);
    r.drawLimb(qa + vec2(0.012f, 0.0f), qb + vec2(0.01f, 0.0f), 0.025f, 0.02f, LEATHER_DK, upper, red);
    band(r, upper, qa, qb, 0.12f, 0.12f, 0.03f, GOLD_DARK, red);
    band(r, upper, qa, qb, 0.55f, 0.115f, 0.022f, GOLD, red);
    band(r, upper, qa, qb, 0.93f, 0.108f, 0.03f, GOLD_DARK, red);
    r.drawEllipse(qb + vec2(0.0f, -0.005f), vec2(0.045f, 0.012f), BLACK, upper, red, 16);   // dark mouth

    // ----------------------------------------------------------- neck + torso
    r.drawRect({0.012f, 0.835f}, {0.05f, 0.075f}, SKIN_DARK, upper, red);

    r.drawConvex({{-0.135f, 0.805f}, {0.135f, 0.805f}, {0.115f, 0.70f}, {-0.115f, 0.70f}}, SKIN, upper, red);  // bare shoulders
    r.drawConvex({{-0.108f, 0.80f}, {0.108f, 0.80f}, {0.105f, 0.525f}, {-0.105f, 0.525f}}, LINEN, upper, red); // linen top
    r.drawConvex({{-0.108f, 0.80f}, {-0.04f, 0.80f}, {-0.045f, 0.525f}, {-0.105f, 0.525f}}, LINEN_DARK, upper, red);
    for (int k = 0; k < 3; ++k)                                                                                // pleats
        r.drawRect({0.02f + k * 0.03f, 0.66f}, {0.006f, 0.25f}, LINEN_DARK, upper, red);
    r.drawEllipse({0.05f, 0.665f}, {0.045f, 0.06f}, LINEN, upper, red);   // chest volume
    r.drawRect({0.02f, 0.54f}, {0.16f, 0.016f}, LINEN_DARK, upper, red);

    // hips + belt + pouch
    r.drawConvex({{-0.115f, 0.53f}, {0.115f, 0.53f}, {0.125f, 0.43f}, {-0.125f, 0.43f}}, TROUSER, upper, red);
    r.drawRect({0.0f, 0.535f}, {0.235f, 0.042f}, LEATHER, upper, red);
    r.drawRect({0.0f, 0.545f}, {0.235f, 0.008f}, LEATHER_DK, upper, red);
    r.drawRect({0.03f, 0.535f}, {0.048f, 0.036f}, GOLD, upper, red);
    r.drawRect({0.03f, 0.535f}, {0.024f, 0.018f}, GOLD_DARK, upper, red);
    r.drawConvex({{-0.115f, 0.52f}, {-0.065f, 0.52f}, {-0.07f, 0.45f}, {-0.112f, 0.45f}}, LEATHER_DK, upper, red);
    r.drawEllipse({-0.09f, 0.51f}, {0.006f, 0.006f}, GOLD, upper, red, 8);

    // baldric: leather strap from right shoulder to left hip
    r.drawLimb({0.085f, 0.805f}, {-0.095f, 0.545f}, 0.034f, 0.034f, LEATHER_DK, upper, red);
    for (int k = 0; k < 4; ++k) {
        float t = 0.18f + k * 0.2f;
        r.drawEllipse(glm::mix(vec2(0.085f, 0.805f), vec2(-0.095f, 0.545f), t), vec2(0.007f), GOLD, upper, red, 8);
    }

    // wide gold collar (usekh) with turquoise + lapis beads
    r.drawEllipse({0.012f, 0.79f}, {0.100f, 0.048f}, GOLD, upper, red, 28);
    r.drawEllipse({0.012f, 0.80f}, {0.062f, 0.030f}, SKIN_DARK, upper, red, 24);
    for (int k = 0; k < 7; ++k) {
        float a = 3.14159f + 3.14159f * (k + 0.5f) / 7.0f;
        r.drawEllipse(vec2(0.012f + std::cos(a) * 0.082f, 0.79f + std::sin(a) * 0.036f), vec2(0.0095f),
                      k % 2 ? LAPIS : TURQUOISE, upper, red, 10);
    }

    // ----------------------------------------------- back (string) arm
    float nockX = TIP_X - PULL * draw + vib * 0.014f;
    vec2 S(-0.055f, 0.785f), H(nockX, HAND_Y + 0.005f);
    vec2 Eext = ik(S, H, 0.16f, 0.17f, 1.0f);
    vec2 Efull(-0.150f, 0.835f);
    vec2 E = glm::mix(Eext, Efull, draw);
    r.drawLimb(S, E, 0.068f, 0.058f, SKIN_DARK, upper, red);
    r.drawLimb(E, H, 0.056f, 0.044f, SKIN_DARK, upper, red);
    r.drawEllipse(S, vec2(0.04f), SKIN_DARK, upper, red, 16);
    band(r, upper, S, E, 0.55f, 0.07f, 0.016f, GOLD, red);   // upper-arm band

    // ------------------------------------------------------------ head
    float sway = std::sin(time_ * 2.4f);
    mat4 head = at(upper, 0.0f, 0.0f);
    r.drawEllipse({-0.02f, 0.925f}, {0.085f, 0.088f}, HAIR, head, red, 24);              // hair mass
    // braid: flows backward, trails the walk
    vec2 prev(-0.07f, 0.93f);
    for (int k = 0; k < 6; ++k) {
        float lag = walkBlend_ * 0.010f * k * std::sin(phase_ * 2.0f) + 0.004f * k * sway;
        vec2 p(-0.080f - 0.020f * k - 0.002f * k * k, 0.915f - 0.038f * k + lag);
        r.drawEllipse(p, vec2(0.036f - 0.0035f * k, 0.030f - 0.002f * k), k % 2 ? HAIR : HAIR_LIGHT, head, red, 14);  // braid links
        prev = p;
    }
    r.drawEllipse(prev + vec2(-0.004f, -0.034f), vec2(0.014f), GOLD, head, red, 10);     // gold hair tie
    r.drawTriangle(prev + vec2(-0.02f, -0.03f), prev + vec2(0.012f, -0.03f), prev + vec2(-0.004f, -0.085f), HAIR, head, red);  // tassel
    r.drawEllipse({-0.068f, 0.935f}, vec2(0.018f), GOLD, head, red, 12);                // tie at head

    r.drawEllipse({0.015f, 0.905f}, {0.066f, 0.080f}, SKIN_LIGHT, head, red, 28);       // face
    r.drawEllipse({-0.012f, 0.895f}, {0.018f, 0.026f}, SKIN, head, red, 12);            // ear
    r.drawEllipse({-0.012f, 0.875f}, {0.007f, 0.011f}, GOLD, head, red, 8);             // earring
    r.drawEllipse({0.0f, 0.968f}, {0.078f, 0.040f}, HAIR, head, red, 24);               // hair top
    r.drawTriangle({0.02f, 0.968f}, {0.082f, 0.95f}, {0.07f, 0.935f}, HAIR, head, red); // side fringe
    r.drawLimb({-0.065f, 0.946f}, {0.078f, 0.954f}, 0.018f, 0.018f, GOLD, head, red);   // golden headband
    r.drawTriangle({0.074f, 0.949f}, {0.098f, 0.968f}, {0.09f, 0.945f}, GOLD, head, red);// small cobra crest
    r.drawEllipse({0.0f, 0.951f}, vec2(0.009f), TURQUOISE, head, red, 10);

    r.drawEllipse({0.044f, 0.918f}, {0.019f, 0.012f}, FEATHER_W, head, red, 14);        // eye
    r.drawEllipse({0.050f, 0.918f}, vec2(0.0085f), BLACK, head, red, 12);
    r.drawLimb({0.022f, 0.923f}, {0.064f, 0.929f}, 0.007f, 0.007f, BLACK, head, red);   // kohl line
    r.drawTriangle({0.060f, 0.927f}, {0.084f, 0.936f}, {0.064f, 0.920f}, BLACK, head, red);
    r.drawLimb({0.028f, 0.942f}, {0.064f, 0.946f}, 0.008f, 0.006f, HAIR, head, red);    // brow
    r.drawTriangle({0.074f, 0.905f}, {0.092f, 0.884f}, {0.066f, 0.884f}, SKIN, head, red);   // nose
    r.drawEllipse({0.056f, 0.863f}, {0.016f, 0.0065f}, LIPS, head, red, 12);            // mouth
    r.drawEllipse({0.036f, 0.885f}, {0.014f, 0.006f}, SKIN_DARK, head, red, 10);        // cheek shade

    // ------------------------------------------------------- front (bow) arm + bow
    vec2 Sb(0.085f, 0.790f), Hb(GRIP_X, HAND_Y);
    vec2 Eb = ik(Sb, Hb, 0.14f, 0.14f, -1.0f);

    // bow limbs (grip in the middle bulges toward the target)
    const int N = 16;
    vec2 pts[N + 1]; float wid[N + 1];
    for (int i = 0; i <= N; ++i) {
        float t = -1.0f + 2.0f * i / N;
        pts[i] = vec2(GRIP_X - BOW_BELLY * t * t, HAND_Y + BOW_HALF * t);
        wid[i] = 0.011f + 0.022f * (1.0f - std::fabs(t));
    }
    for (int i = 0; i < N; ++i)
        r.drawLimb(pts[i], pts[i + 1], wid[i], wid[i + 1], WOOD, upper, red);
    for (int i = 1; i < N; ++i)   // gold inlay along the limbs
        if (i % 2 == 0 && std::abs(i - N / 2) > 1)
            r.drawEllipse(pts[i], vec2(0.0055f), GOLD, upper, red, 8);
    for (int i = 0; i < N; ++i)   // dark edge line
        r.drawLimb(pts[i] + vec2(-0.006f, 0.0f), pts[i + 1] + vec2(-0.006f, 0.0f), 0.004f, 0.004f, WOOD_DARK, upper, red);
    r.drawEllipse(pts[0],  vec2(0.013f), GOLD, upper, red, 10);   // gilded tips
    r.drawEllipse(pts[N],  vec2(0.013f), GOLD, upper, red, 10);
    r.drawRect({GRIP_X - 0.004f, HAND_Y}, {0.04f, 0.105f}, LEATHER_DK, upper, red);   // leather grip
    for (int k = 0; k < 4; ++k)
        r.drawRect({GRIP_X - 0.004f, HAND_Y - 0.038f + k * 0.025f}, {0.042f, 0.006f}, GOLD_DARK, upper, red);

    // bow string
    vec2 Tt(TIP_X, HAND_Y + BOW_HALF), Tb(TIP_X, HAND_Y - BOW_HALF), N0(nockX, HAND_Y + 0.005f);
    r.drawLimb(Tt, N0, 0.006f, 0.006f, STRING_COL, upper, red);
    r.drawLimb(N0, Tb, 0.006f, 0.006f, STRING_COL, upper, red);

    // nocked arrow (only while drawing)
    if (cycle_ >= DRAW_START && cycle_ < RELEASE)
        arrowShape(r, upper, N0 + vec2(ARROW_HALF, 0.0f), 1.0f);

    // bow arm (drawn over the grip), bracer, hand
    r.drawLimb(Sb, Eb, 0.068f, 0.058f, SKIN, upper, red);
    r.drawLimb(Eb, Hb, 0.058f, 0.046f, SKIN, upper, red);
    r.drawEllipse(Sb, vec2(0.042f), SKIN, upper, red, 16);
    band(r, upper, Sb, Eb, 0.50f, 0.07f, 0.018f, GOLD, red);                          // armband
    band(r, upper, Eb, Hb, 0.62f, 0.06f, 0.09f, LEATHER, red);                         // archer's bracer
    band(r, upper, Eb, Hb, 0.40f, 0.062f, 0.012f, GOLD_DARK, red);
    band(r, upper, Eb, Hb, 0.84f, 0.056f, 0.012f, GOLD_DARK, red);
    r.drawEllipse(Hb + vec2(0.004f, 0.0f), {0.034f, 0.030f}, SKIN, upper, red, 16);
    for (int k = 0; k < 3; ++k)   // fingers curled round the grip
        r.drawEllipse(Hb + vec2(0.030f, 0.018f - k * 0.018f), vec2(0.011f), SKIN_LIGHT, upper, red, 8);

    // draw hand on the string
    r.drawEllipse(H, {0.030f, 0.024f}, SKIN_DARK, upper, red, 16);
    r.drawEllipse(H + vec2(0.012f, 0.012f), vec2(0.010f), SKIN, upper, red, 8);

}

void LongRangePlayer::drawEffects(Renderer& renderer) const
{
    Renderer2D r(renderer);
    for (const Arrow& arrow : arrows_) drawArrow(r, arrow);
    if (hp_ <= 0) return;
    const mat4 base = glm::scale(glm::translate(mat4(1.0f), glm::vec3(pos_, 0.0f)),
                                 glm::vec3(height_, height_, 1.0f));
    drawHealthBar(r, base);
}

// ------------------------------------------------------------ arrow in flight
void LongRangePlayer::drawArrow(Renderer2D& r, const Arrow& a) const
{
    mat4 m = glm::translate(mat4(1.0f), glm::vec3(a.pos, 0.0f));
    if (!a.hit) {
        arrowShape(r, glm::scale(m, glm::vec3(height_, height_, 1.0f)), vec2(0.0f), 1.0f);
        return;
    }
    // impact: the arrow shudders and fades, with a burst of golden sparks at the tip
    float u = std::min(1.0f, a.hitTime / IMPACT_TIME);
    mat4 s = glm::scale(m, glm::vec3(height_, height_, 1.0f));
    mat4 tip = at(s, ARROW_HALF, 0.0f);
    mat4 shudder = at(tip, 0.0f, 0.0f, std::sin(a.hitTime * 80.0f) * 7.0f * (1.0f - u));
    arrowShape(r, shudder, vec2(-ARROW_HALF, 0.0f), 1.0f - u);
    for (int k = 0; k < 8; ++k) {
        float ang = glm::radians(45.0f * k + 22.0f);
        mat4 sp = at(tip, 0.0f, 0.0f, glm::degrees(ang));
        float len = 0.035f + 0.11f * u;
        r.drawTriangle({0.03f + len, 0.0f}, {0.03f + len * 0.35f, 0.012f * (1.0f - u)},
                       {0.03f + len * 0.35f, -0.012f * (1.0f - u)}, alpha(k % 2 ? GOLD : FEATHER_W, 1.0f - u), sp);
    }
}

// ----------------------------------------------------------------- health bar
void LongRangePlayer::drawHealthBar(Renderer2D& r, const mat4& m) const
{
    const int   n = MAX_HEALTH;
    const float segW = 0.11f, gap = 0.016f;
    const float total = n * segW + (n - 1) * gap;
    const float cx = 0.0f, cy = 1.13f + jumpOffset();

    float frac = (float)hp_ / (float)n;
    vec4 fill = frac > 0.5f ? vec4(0.20f, 0.80f, 0.20f, 1.0f)
              : frac > 0.25f ? vec4(0.95f, 0.80f, 0.15f, 1.0f)
                             : vec4(0.90f, 0.15f, 0.15f, 1.0f);

    r.drawRect({cx, cy}, {total + 0.03f, 0.065f}, vec4(0.08f, 0.08f, 0.08f, 1.0f), m);
    for (int i = 0; i < n; ++i) {
        float x = cx - total * 0.5f + segW * 0.5f + i * (segW + gap);
        r.drawRect({x, cy}, {segW, 0.04f}, i < hp_ ? fill : vec4(0.28f, 0.28f, 0.28f, 1.0f), m);
    }
}
