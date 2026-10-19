#include "MeleePlayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

using glm::vec2;
using glm::vec4;
using glm::mat4;

namespace {
// ---- Palette: "Galabeya Fighter" (dusty cream robe, checkered keffiyeh, leather, olive gloves, steel) ----
const vec4 SKIN       (0.78f, 0.52f, 0.36f, 1.0f);
const vec4 SKIN_LIGHT (0.87f, 0.62f, 0.45f, 1.0f);
const vec4 SKIN_DARK  (0.56f, 0.35f, 0.24f, 1.0f);
const vec4 BEARD      (0.17f, 0.12f, 0.10f, 1.0f);

const vec4 ROBE       (0.89f, 0.84f, 0.72f, 1.0f);   // galabeya
const vec4 ROBE_LITE  (0.96f, 0.93f, 0.84f, 1.0f);
const vec4 ROBE_SHADE (0.74f, 0.67f, 0.53f, 1.0f);
const vec4 ROBE_DARK  (0.55f, 0.47f, 0.35f, 1.0f);
const vec4 DIRT       (0.55f, 0.42f, 0.28f, 1.0f);   // desert stains

const vec4 KEFF       (0.82f, 0.75f, 0.62f, 1.0f);   // keffiyeh
const vec4 KEFF_LITE  (0.91f, 0.86f, 0.74f, 1.0f);
const vec4 KEFF_DARK  (0.33f, 0.27f, 0.21f, 1.0f);

const vec4 LEATHER    (0.38f, 0.26f, 0.17f, 1.0f);
const vec4 LEATHER_DK (0.22f, 0.15f, 0.10f, 1.0f);
const vec4 LEATHER_LT (0.53f, 0.39f, 0.27f, 1.0f);
const vec4 POUCH      (0.43f, 0.36f, 0.24f, 1.0f);
const vec4 POUCH_DK   (0.30f, 0.25f, 0.16f, 1.0f);

const vec4 GLOVE      (0.21f, 0.23f, 0.15f, 1.0f);   // fingerless olive gloves
const vec4 GLOVE_LT   (0.33f, 0.35f, 0.24f, 1.0f);

const vec4 BOOT       (0.31f, 0.23f, 0.17f, 1.0f);   // combat boots
const vec4 BOOT_LITE  (0.47f, 0.36f, 0.26f, 1.0f);
const vec4 SOLE       (0.11f, 0.09f, 0.08f, 1.0f);

const vec4 STEEL      (0.55f, 0.57f, 0.61f, 1.0f);   // 1911s
const vec4 STEEL_DARK (0.30f, 0.32f, 0.36f, 1.0f);
const vec4 STEEL_LITE (0.76f, 0.78f, 0.80f, 1.0f);
const vec4 GRIP       (0.52f, 0.29f, 0.19f, 1.0f);
const vec4 GRIP_DK    (0.35f, 0.19f, 0.12f, 1.0f);

const vec4 INK        (0.13f, 0.09f, 0.07f, 1.0f);   // outline colour
const vec4 BLACK      (0.05f, 0.04f, 0.04f, 1.0f);
const vec4 WHITE      (0.96f, 0.95f, 0.92f, 1.0f);
const vec4 GOLD       (0.95f, 0.76f, 0.22f, 1.0f);
const vec4 FIRE       (0.98f, 0.55f, 0.10f, 1.0f);

const float HIT_TIME    = 0.35f;
const float JUMP_TIME   = 0.70f;
const float DUCK_TIME   = 0.70f;
const float JUMP_HEIGHT = 0.38f;   // model units
const float DUCK_DROP   = 0.15f;
const float CYCLE       = 0.90f;   // seconds between two volleys
const float FIRE_AT     = 0.35f;   // the bang (front pistol; this is the one that deals damage)
const float SECOND_GUN  = 0.12f;   // the second pistol goes off this much later (visual only)
const float FLASH_TIME  = 0.09f;
const float SMOKE_TIME  = 0.55f;
const float WAIST       = 0.50f;   // pivot height for leaning
const float BULLET_SPEED = 2.2f;   // character heights per second
const float BULLET_DROP  = 4.0f;   // gravity once out of range (heights per second^2)
const float BULLET_REST  = 0.50f;  // seconds a spent bullet lies on the ground before vanishing
const float OUT         = 0.006f;  // outline thickness (model units)

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

vec4 alpha(vec4 c, float a) { c.a = a; return c; }
vec2 rot(vec2 v, float deg)
{
    float c = std::cos(glm::radians(deg)), s = std::sin(glm::radians(deg));
    return vec2(c * v.x - s * v.y, s * v.x + c * v.y);
}

vec2 safeDir(vec2 d)
{
    float l = glm::length(d);
    return l < 1e-6f ? vec2(1.0f, 0.0f) : d / l;
}

// ---------------------------------------------------------------- outlined shapes
// Grow a convex polygon outwards by d (mitred corners). Used to draw the dark ink line.
std::vector<vec2> grow(const std::vector<vec2>& p, float d)
{
    size_t n = p.size();
    float area = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        vec2 a = p[i], b = p[(i + 1) % n];
        area += a.x * b.y - b.x * a.y;
    }
    float sgn = area > 0.0f ? 1.0f : -1.0f;
    std::vector<vec2> q(n);
    for (size_t i = 0; i < n; ++i) {
        vec2 a = p[(i + n - 1) % n], b = p[i], c = p[(i + 1) % n];
        vec2 e1 = safeDir(b - a), e2 = safeDir(c - b);
        vec2 n1 = vec2(e1.y, -e1.x) * sgn, n2 = vec2(e2.y, -e2.x) * sgn;
        vec2 m = n1 + n2;
        float l = glm::length(m);
        m = l < 1e-4f ? n1 : m / l;
        float k = std::max(0.5f, glm::dot(m, n1));
        q[i] = b + m * (d / k);
    }
    return q;
}

// polygon with a dark outline
void shape(Renderer2D& r, const mat4& m, const std::vector<vec2>& p, const vec4& col, float red, float d = OUT)
{
    r.drawConvex(grow(p, d), INK, m, red);
    r.drawConvex(p, col, m, red);
}

// tapered limb as a plain quad (no round caps)
std::vector<vec2> limbPts(vec2 a, vec2 b, float w0, float w1)
{
    vec2 d = safeDir(b - a), n(-d.y, d.x);
    return {a - n * (w0 * 0.5f), b - n * (w1 * 0.5f), b + n * (w1 * 0.5f), a + n * (w0 * 0.5f)};
}
void limb(Renderer2D& r, const mat4& m, vec2 a, vec2 b, float w0, float w1, const vec4& col, float red, float d = OUT)
{
    shape(r, m, limbPts(a, b, w0, w1), col, red, d);
}
// un-outlined limb (shading lines, stripes)
void stroke(Renderer2D& r, const mat4& m, vec2 a, vec2 b, float w0, float w1, const vec4& col, float red)
{
    r.drawConvex(limbPts(a, b, w0, w1), col, m, red);
}

std::vector<vec2> rectPts(vec2 c, vec2 s)
{
    vec2 h = s * 0.5f;
    return {c + vec2(-h.x, -h.y), c + vec2(h.x, -h.y), c + vec2(h.x, h.y), c + vec2(-h.x, h.y)};
}

// cuff / rolled sleeve / glove wrist across a limb a->b
void band(Renderer2D& r, const mat4& m, vec2 a, vec2 b, float t, float across, float thick,
          const vec4& col, float red)
{
    vec2 d = safeDir(b - a), n(-d.y, d.x), p = a + (b - a) * t;
    vec2 u = d * thick * 0.5f, v = n * across * 0.5f;
    shape(r, m, {p - u - v, p + u - v, p + u + v, p - u + v}, col, red, 0.004f);
}

// Keffiyeh weave: a fine, low-contrast grid of thin lines kept inside an ellipse.
void weave(Renderer2D& r, const mat4& m, vec2 c, vec2 rad, float sp, const vec4& col, float red)
{
    const float sh = 0.88f;
    for (float x = c.x - rad.x * sh + sp * 0.5f; x < c.x + rad.x * sh; x += sp) {
        float k = (x - c.x) / rad.x;
        float h = rad.y * std::sqrt(std::max(0.0f, 1.0f - k * k)) * sh;
        r.drawRect({x, c.y}, {0.004f, 2.0f * h}, col, m, red);
    }
    for (float y = c.y - rad.y * sh + sp * 0.5f; y < c.y + rad.y * sh; y += sp) {
        float k = (y - c.y) / rad.y;
        float w = rad.x * std::sqrt(std::max(0.0f, 1.0f - k * k)) * sh;
        r.drawRect({c.x, y}, {2.0f * w, 0.004f}, col, m, red);
    }
}

// irregular smudge of desert dust (unit blob scaled by sx, sy)
void smudge(Renderer2D& r, const mat4& m, vec2 c, float sx, float sy, float a, float red)
{
    static const vec2 B[6] = {{-1.0f, 0.1f}, {-0.4f, 0.8f}, {0.5f, 0.9f}, {1.0f, 0.2f}, {0.6f, -0.7f}, {-0.5f, -0.8f}};
    std::vector<vec2> p;
    for (const vec2& b : B) p.push_back(c + vec2(b.x * sx, b.y * sy));
    r.drawConvex(p, alpha(DIRT, a), m, red);
}

// A weathered 1911. Origin = where the hand holds the grip, barrel along +x. Muzzle at (0.168, 0.045).
const vec2 MUZZLE(0.168f, 0.045f);
const vec2 EJECT (0.030f, 0.066f);

void drawPistol(Renderer2D& r, const mat4& g, float red)
{
    const float d = 0.005f;
    shape(r, g, {{-0.045f, 0.016f}, {0.012f, 0.020f}, {0.002f, -0.095f}, {-0.060f, -0.088f}}, GRIP, red, d);   // wooden grip
    r.drawConvex({{-0.034f, 0.006f}, {0.002f, 0.009f}, {-0.006f, -0.078f}, {-0.050f, -0.072f}}, GRIP_DK, g, red);
    shape(r, g, rectPts({0.030f, 0.019f}, {0.170f, 0.014f}), STEEL_DARK, red, d);                               // frame
    shape(r, g, {{0.022f, 0.014f}, {0.042f, -0.014f}, {0.068f, 0.014f}}, STEEL_DARK, red, 0.003f);              // trigger guard
    shape(r, g, rectPts({0.052f, 0.045f}, {0.222f, 0.040f}), STEEL, red, d);                                    // slide
    r.drawRect({0.052f, 0.061f}, {0.214f, 0.007f}, STEEL_LITE, g, red);
    r.drawRect({0.034f, 0.048f}, {0.056f, 0.014f}, alpha(BLACK, 0.60f), g, red);                                // ejection port
    shape(r, g, rectPts({0.161f, 0.045f}, {0.012f, 0.034f}), STEEL_DARK, red, 0.003f);                          // muzzle ring
    shape(r, g, {{0.146f, 0.064f}, {0.156f, 0.064f}, {0.152f, 0.078f}}, STEEL_DARK, red, 0.003f);               // front sight
    shape(r, g, rectPts({-0.047f, 0.072f}, {0.016f, 0.010f}), STEEL_DARK, red, 0.003f);                         // rear sight
    shape(r, g, {{-0.062f, 0.052f}, {-0.050f, 0.070f}, {-0.082f, 0.060f}}, STEEL_DARK, red, 0.003f);            // hammer
}

// Gloved hand gripping the pistol (drawn in the gun's frame so it recoils with it).
void drawHand(Renderer2D& r, const mat4& g, float red)
{
    const float d = 0.005f;
    shape(r, g, {{-0.078f, 0.020f}, {-0.020f, 0.036f}, {0.010f, 0.010f}, {0.008f, -0.074f}, {-0.050f, -0.086f}, {-0.082f, -0.040f}}, GLOVE, red, d);
    r.drawConvex({{-0.066f, 0.014f}, {-0.030f, 0.024f}, {-0.034f, -0.010f}, {-0.066f, -0.012f}}, GLOVE_LT, g, red);   // knuckle plate
    shape(r, g, {{0.004f, 0.002f}, {0.032f, 0.000f}, {0.034f, -0.072f}, {0.004f, -0.074f}}, SKIN_LIGHT, red, 0.004f); // fingers
    stroke(r, g, {0.004f, -0.026f}, {0.033f, -0.026f}, 0.003f, 0.003f, INK, red);
    stroke(r, g, {0.004f, -0.050f}, {0.033f, -0.050f}, 0.003f, 0.003f, INK, red);
    shape(r, g, {{-0.034f, 0.034f}, {0.018f, 0.036f}, {0.022f, 0.016f}, {-0.030f, 0.012f}}, GLOVE, red, 0.004f);      // thumb
}

// Muzzle flash + smoke ("the bang"). ts = seconds since this gun fired.
void drawBang(Renderer2D& r, const mat4& upper, vec2 H, float angDeg, float ts)
{
    if (ts < 0.0f || ts >= SMOKE_TIME) return;
    vec2 M = H + rot(MUZZLE, angDeg);
    if (ts < FLASH_TIME) {
        float u = ts / FLASH_TIME;
        mat4 f = at(upper, M.x, M.y, angDeg);
        float L = 0.20f * (1.0f - 0.4f * u);
        r.drawTriangle({0.0f, 0.050f}, {0.0f, -0.050f}, {L + 0.08f, 0.0f}, alpha(GOLD, 0.95f), f);
        r.drawTriangle({0.0f, 0.030f}, {0.05f, 0.11f}, {0.09f, 0.02f}, alpha(GOLD, 0.9f), f);
        r.drawTriangle({0.0f, -0.030f}, {0.05f, -0.11f}, {0.09f, -0.02f}, alpha(GOLD, 0.9f), f);
        r.drawTriangle({0.0f, 0.0f}, {0.07f, 0.05f}, {0.15f, 0.07f}, alpha(FIRE, 0.85f), f);
        r.drawTriangle({0.0f, 0.0f}, {0.07f, -0.05f}, {0.15f, -0.07f}, alpha(FIRE, 0.85f), f);
        r.drawEllipse({0.02f, 0.0f}, {0.05f, 0.032f}, alpha(WHITE, 1.0f), f, 0.0f, 14);
    }
    float u = ts / SMOKE_TIME;
    for (int k = 0; k < 3; ++k) {
        float v = std::min(1.0f, u * (1.0f + 0.25f * k));
        vec2 p = M + vec2(0.03f + 0.07f * v + k * 0.02f, 0.02f + 0.11f * v + k * 0.012f);
        r.drawEllipse(p, vec2(0.022f + 0.04f * v), alpha(vec4(0.82f, 0.80f, 0.76f, 1.0f), 0.55f * (1.0f - v)), upper, 0.0f, 16);
    }
}
} // namespace

MeleePlayer::MeleePlayer(vec2 feetPos, float height)
    : pos_(feetPos), height_(height), speed_(0.40f * height) {}

float MeleePlayer::jumpOffset() const
{
    if (jumpT_ <= 0.0f) return 0.0f;
    float u = 1.0f - jumpT_ / JUMP_TIME;
    return JUMP_HEIGHT * 4.0f * u * (1.0f - u);
}

float MeleePlayer::duckAmount() const
{
    if (duckT_ <= 0.0f) return 0.0f;
    float u = 1.0f - duckT_ / DUCK_TIME;
    return smooth(std::min(u, 1.0f - u) / 0.15f);
}

float MeleePlayer::aimAmount() const
{
    if (cycle_ < FIRE_AT) return 0.4f + 0.6f * smooth(cycle_ / 0.30f);
    float t = cycle_ - FIRE_AT;
    return 1.0f - 0.6f * smooth((t - 0.10f) / 0.45f);
}

float MeleePlayer::sinceShot() const { return cycle_ - FIRE_AT; }

bool MeleePlayer::consumeAttack()
{
    bool a = pendingAttack_;
    pendingAttack_ = false;
    return a;
}

void MeleePlayer::jump()
{
    if (hp_ > 0 && jumpT_ <= 0.0f && duckT_ <= 0.0f) jumpT_ = JUMP_TIME;
}

void MeleePlayer::duck()
{
    if (hp_ > 0 && jumpT_ <= 0.0f && duckT_ <= 0.0f) duckT_ = DUCK_TIME;
}

void MeleePlayer::takeHit()
{
    if (hp_ <= 0) return;
    --hp_;
    hitTimer_ = HIT_TIME;
}

void MeleePlayer::reset()
{
    bullets_.clear();
    hp_ = MAX_HEALTH;
    hitTimer_ = 0.0f;
    cycle_ = 0.10f;
    fired_ = false;
    pendingAttack_ = false;
    jumpT_ = duckT_ = 0.0f;
}

// ---------------------------------------------------------------- bullets
void MeleePlayer::spawnBullet()
{
    // muzzle of the front pistol at the moment of the bang (aim 1, full recoil), upper-body space ...
    float lean = -4.0f - 3.0f + 2.5f - 3.0f * walkBlend_;
    vec2 H(0.430f - 0.025f, 0.775f + 0.014f);
    vec2 M = H + rot(MUZZLE, 14.0f);
    // ... converted to root space (lean pivots at the waist), then to world space
    vec2 mr = vec2(0.0f, WAIST - DUCK_DROP * duckAmount()) + rot(M - vec2(0.0f, WAIST), lean);
    Bullet b;
    b.p = b.prev = pos_ + vec2(mr.x, mr.y + jumpOffset()) * height_;
    b.vx = BULLET_SPEED * height_;
    b.vy = 0.0f;
    bullets_.push_back(b);
}

void MeleePlayer::updateBullets(float dt)
{
    for (Bullet& b : bullets_) {
        b.prev = b.p;
        if (b.landed) { b.rest += dt; continue; }
        if (!b.dropping) {                                   // flying straight
            b.p.x += b.vx * dt;
            b.travelled += b.vx * dt;
            if (b.travelled >= bulletRange()) { b.dropping = true; b.vx *= 0.35f; }   // out of range
        } else {                                             // lost its punch: falls to the ground
            b.vy -= BULLET_DROP * height_ * dt;
            b.vx *= std::max(0.0f, 1.0f - 2.0f * dt);
            b.p += vec2(b.vx, b.vy) * dt;
            float ground = pos_.y + 0.012f * height_;
            if (b.p.y <= ground) { b.p.y = ground; b.vx = b.vy = 0.0f; b.landed = true; }
        }
    }
    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                  [](const Bullet& b) { return b.landed && b.rest >= BULLET_REST; }),
                   bullets_.end());
}

bool MeleePlayer::bulletHits(float xMin, float xMax)
{
    for (size_t i = 0; i < bullets_.size(); ++i) {
        const Bullet& b = bullets_[i];
        if (b.dropping || b.landed) continue;                // only bullets still in flight can hurt
        float lo = std::min(b.prev.x, b.p.x), hi = std::max(b.prev.x, b.p.x);
        if (hi >= xMin && lo <= xMax) {                      // swept test: no tunnelling at low frame rates
            bullets_.erase(bullets_.begin() + i);
            return true;
        }
    }
    return false;
}

void MeleePlayer::update(float dt)
{
    time_ += dt;
    updateBullets(dt);                                       // bullets keep flying even if he just died
    if (hitTimer_ > 0.0f) hitTimer_ = std::max(0.0f, hitTimer_ - dt);
    if (hp_ <= 0) return;

    // shoot cycle: raise -> bang -> recoil -> lower, forever (also while walking)
    cycle_ += dt;
    if (!fired_ && cycle_ >= FIRE_AT) { fired_ = true; pendingAttack_ = true; spawnBullet(); }
    if (cycle_ >= CYCLE) { cycle_ -= CYCLE; fired_ = false; }

    if (jumpT_ > 0.0f) jumpT_ = std::max(0.0f, jumpT_ - dt);
    if (duckT_ > 0.0f) duckT_ = std::max(0.0f, duckT_ - dt);

    bool moving = !engaged_;
    walkBlend_ += ((moving ? 1.0f : 0.0f) - walkBlend_) * std::min(1.0f, dt * 8.0f);
    if (moving) { pos_.x += speed_ * dt; phase_ += dt * 7.0f; }
}

// ------------------------------------------------------------------
// Hierarchy:  base (world position + scale)
//               root  (+ hit shake, jump)     boots, robe skirt
//                 upper (+ breathing, duck, lean)   holster, torso, rig, arms + pistols, head
// Every solid part is drawn with a dark ink outline (shape / limb helpers) so the pieces read as one drawing.
// ------------------------------------------------------------------
void MeleePlayer::draw(Renderer2D& r) const
{
    if (hp_ <= 0) { drawBullets(r); return; }   // defeated -> removed from the scene (bullets finish their flight)

    float flash = hitTimer_ / HIT_TIME;
    float shake = std::sin(hitTimer_ * 70.0f) * 0.02f * flash;
    float bob   = std::sin(time_ * 2.4f) * 0.005f + walkBlend_ * std::fabs(std::sin(phase_)) * 0.012f;
    float lost  = 1.0f - (float)hp_ / (float)MAX_HEALTH;
    float red   = std::min(1.0f, lost * 0.75f + flash * 0.25f);

    float ja   = jumpOffset();
    float da   = duckAmount();
    float drop = DUCK_DROP * da;
    float aim  = aimAmount();
    float t    = sinceShot();                       // front pistol
    float t2   = t - SECOND_GUN;                    // second pistol, a beat later
    float kick  = t  >= 0.0f ? std::exp(-t  * 9.0f) : 0.0f;
    float kick2 = t2 >= 0.0f ? std::exp(-t2 * 9.0f) : 0.0f;

    mat4 base  = glm::scale(glm::translate(mat4(1.0f), glm::vec3(pos_, 0.0f)),
                            glm::vec3(height_, height_, 1.0f));
    mat4 root  = at(base, shake, ja);

    // upper body leans into the shot (pivot at the waist)
    float lean = -4.0f - 3.0f * aim + 2.5f * kick - 3.0f * walkBlend_;
    mat4 upper = at(root, 0.0f, WAIST + bob - drop, lean);
    upper = glm::translate(upper, glm::vec3(0.0f, -WAIST, 0.0f));

    // ----------------------------------------------------------- combat boots (the robe hides the legs)
    float jam = std::min(1.0f, ja / JUMP_HEIGHT);
    vec2 ank[2];
    for (int i = 0; i < 2; ++i) {
        float s = std::sin(phase_ + i * 3.14159f), c = std::cos(phase_ + i * 3.14159f);
        vec2 stance = i == 0 ? vec2(-0.22f, 0.06f) : vec2(0.22f, 0.06f);          // wide fighting stance
        vec2 walk(-0.02f + (i ? 0.04f : 0.0f) + 0.12f * s, 0.06f + 0.05f * std::max(0.0f, c));
        ank[i] = glm::mix(stance, walk, walkBlend_);
        ank[i].y += 0.14f * jam;
        ank[i].x += (i ? 0.05f : -0.03f) * jam;
    }
    for (int i = 0; i < 2; ++i) {
        vec2 a = ank[i];
        std::vector<vec2> shaft = {a + vec2(-0.052f, 0.15f), a + vec2(0.052f, 0.15f), a + vec2(0.056f, 0.03f), a + vec2(-0.056f, 0.03f)};
        std::vector<vec2> foot  = {a + vec2(-0.056f, -0.050f), a + vec2(0.135f, -0.050f), a + vec2(0.168f, -0.018f),
                                   a + vec2(0.140f, 0.028f), a + vec2(0.070f, 0.052f), a + vec2(-0.050f, 0.070f)};
        r.drawConvex(grow(shaft, OUT), INK, root, red);        // one ink pass for the whole boot,
        r.drawConvex(grow(foot, OUT), INK, root, red);
        r.drawConvex(shaft, BOOT, root, red);                  // then the fills (no seam between shaft and foot)
        r.drawConvex(foot, BOOT, root, red);
        r.drawRect(a + vec2(0.0f, 0.138f), {0.104f, 0.020f}, BOOT_LITE, root, red);          // top cuff
        r.drawRect(a + vec2(0.056f, -0.043f), {0.230f, 0.022f}, SOLE, root, red);            // sole
    }

    // ----------------------------------------------------------- the galabeya (long robe)
    {
        float waist = 0.505f - drop;
        vec2 aB = ank[0].x < ank[1].x ? ank[0] : ank[1];     // back foot
        vec2 aF = ank[0].x < ank[1].x ? ank[1] : ank[0];     // front foot
        vec2 topL(-0.145f, waist), topR(0.125f, waist);
        vec2 hemL(std::min(aB.x - 0.06f, -0.17f), aB.y + 0.14f);
        vec2 hemR(std::max(aF.x + 0.08f,  0.17f), aF.y + 0.14f);
        vec2 hemM = (hemL + hemR) * 0.5f + vec2(0.0f, -0.045f);      // drapes lower in the middle

        auto topAt = [&](float u) { return glm::mix(topL, topR, u); };
        auto hemAt = [&](float u) { return u < 0.5f ? glm::mix(hemL, hemM, u * 2.0f)
                                                    : glm::mix(hemM, hemR, (u - 0.5f) * 2.0f); };
        auto pt    = [&](float u, float v) { return glm::mix(topAt(u), hemAt(u), v); };

        shape(r, root, {topL, topR, hemR, hemM, hemL}, ROBE, red);
        r.drawConvex({topL, topAt(0.40f), hemAt(0.38f), hemL}, ROBE_SHADE, root, red);   // back half in shade

        const float foldU[4] = {0.22f, 0.42f, 0.62f, 0.82f};
        for (int k = 0; k < 4; ++k) {                                                     // a few long drapery folds
            float u = foldU[k];
            float sway = std::sin(time_ * 1.7f + k) * 0.006f * (1.0f - walkBlend_) + walkBlend_ * std::sin(phase_ + k) * 0.012f;
            vec2 a = pt(u, 0.05f), b = pt(u + (k < 2 ? -0.05f : 0.05f), 0.97f) + vec2(sway, 0.0f);
            stroke(r, root, a, b, 0.008f, 0.026f, alpha(ROBE_DARK, 0.30f), red);
        }

        // desert dust: a few big smudges, heaviest near the hem
        smudge(r, root, pt(0.22f, 0.80f), 0.060f, 0.030f, 0.38f, red);
        smudge(r, root, pt(0.70f, 0.84f), 0.070f, 0.034f, 0.40f, red);
        smudge(r, root, pt(0.46f, 0.58f), 0.050f, 0.034f, 0.26f, red);
        smudge(r, root, pt(0.80f, 0.40f), 0.034f, 0.040f, 0.22f, red);
        vec2 up(0.0f, 0.045f);
        r.drawConvex({hemL, hemM, hemM + up, hemL + up}, alpha(DIRT, 0.40f), root, red);
        r.drawConvex({hemM, hemR, hemR + up, hemM + up}, alpha(DIRT, 0.40f), root, red);
    }

    // ------------------------------------------------ holster on the back hip (empty - both guns are drawn)
    {
        mat4 h = at(upper, -0.165f, 0.50f, -14.0f);
        shape(r, h, {{-0.030f, 0.0f}, {0.032f, 0.0f}, {0.036f, -0.150f}, {-0.034f, -0.165f}}, LEATHER, red);
        r.drawConvex({{-0.030f, 0.0f}, {0.032f, 0.0f}, {0.031f, -0.016f}, {-0.030f, -0.016f}}, BLACK, h, red);   // mouth
        stroke(r, h, {-0.034f, -0.060f}, {0.036f, -0.060f}, 0.014f, 0.014f, LEATHER_LT, red);                  // retention strap
    }

    // ----------------------------------------------------------- torso (robe)
    shape(r, upper, {{-0.155f, 0.775f}, {0.120f, 0.775f}, {0.110f, 0.500f}, {-0.140f, 0.500f}}, ROBE, red);
    r.drawConvex({{-0.155f, 0.775f}, {-0.070f, 0.775f}, {-0.075f, 0.500f}, {-0.140f, 0.500f}}, ROBE_SHADE, upper, red);
    stroke(r, upper, {0.050f, 0.735f}, {0.060f, 0.540f}, 0.008f, 0.018f, alpha(ROBE_DARK, 0.30f), red);        // chest folds
    smudge(r, upper, {-0.020f, 0.610f}, 0.040f, 0.024f, 0.28f, red);
    smudge(r, upper, {0.060f, 0.690f}, 0.030f, 0.020f, 0.24f, red);

    // crossed leather bandolier straps + chest pouch
    limb(r, upper, {-0.125f, 0.772f}, {0.088f, 0.512f}, 0.034f, 0.034f, LEATHER, red);
    limb(r, upper, { 0.080f, 0.772f}, {-0.118f, 0.522f}, 0.030f, 0.030f, LEATHER_DK, red);
    {
        mat4 p = at(upper, 0.040f, 0.610f, -18.0f);
        shape(r, p, rectPts({0.0f, 0.0f}, {0.058f, 0.074f}), POUCH_DK, red, 0.005f);
        shape(r, p, rectPts({0.0f, 0.020f}, {0.058f, 0.034f}), POUCH, red, 0.003f);
    }

    // utility belt: buckle in the middle, two ammo pouches either side
    shape(r, upper, rectPts({0.0f, 0.505f}, {0.275f, 0.050f}), LEATHER_DK, red);
    shape(r, upper, rectPts({0.0f, 0.505f}, {0.052f, 0.042f}), STEEL, red, 0.004f);                          // buckle
    r.drawRect({0.0f, 0.505f}, {0.026f, 0.020f}, STEEL_DARK, upper, red);
    shape(r, upper, {{-0.135f, 0.540f}, {-0.058f, 0.540f}, {-0.060f, 0.455f}, {-0.133f, 0.455f}}, POUCH_DK, red, 0.005f);
    shape(r, upper, rectPts({-0.096f, 0.522f}, {0.074f, 0.036f}), POUCH, red, 0.003f);
    shape(r, upper, {{0.058f, 0.540f}, {0.115f, 0.540f}, {0.113f, 0.455f}, {0.060f, 0.455f}}, POUCH_DK, red, 0.005f);
    shape(r, upper, rectPts({0.087f, 0.522f}, {0.056f, 0.036f}), POUCH, red, 0.003f);

    // ------------------------------------------- rear arm: forearm level at belly height, second pistol
    {
        vec2 S(-0.040f, 0.740f);
        vec2 H = glm::mix(vec2(0.120f, 0.470f), vec2(0.175f, 0.615f), aim);
        H += vec2(-0.040f * kick2, 0.016f * kick2);
        float ang = -22.0f * (1.0f - aim) + 12.0f * kick2;
        vec2 W = H + vec2(-0.045f, 0.0f);
        vec2 E = ik(S, W, 0.150f, 0.150f, -1.0f);
        mat4 g = at(upper, H.x, H.y, ang);

        limb(r, upper, E - safeDir(W - E) * 0.04f, W, 0.074f, 0.064f, SKIN, red);          // bare forearm
        limb(r, upper, S, E, 0.095f, 0.088f, ROBE_SHADE, red);                             // sleeve
        band(r, upper, S, E, 0.93f, 0.100f, 0.042f, ROBE, red);                            // rolled cuff
        drawPistol(r, g, red);
        drawHand(r, g, red);
        band(r, upper, E, W, 0.90f, 0.074f, 0.030f, GLOVE, red);                           // glove wrist
        drawBang(r, upper, H, ang, t2);
    }

    // ------------------------------------------------------------ head (keffiyeh, beard, scowl)
    mat4 head = upper;
    float flutter = std::sin(time_ * 5.0f) * 0.014f + walkBlend_ * std::sin(phase_ * 2.0f) * 0.018f;
    float back    = walkBlend_ * 0.035f;

    // scarf tails streaming behind (one ink pass, then the fills, so the outline is continuous)
    {
        vec2 p0(-0.095f, 0.900f), p1(-0.180f - back, 0.890f + flutter * 0.8f),
             p2(-0.265f - back * 1.4f, 0.862f + flutter * 1.5f), p3(-0.345f - back * 1.8f, 0.872f + flutter * 2.2f);
        std::vector<vec2> a1 = limbPts(p0, p1, 0.100f, 0.092f), a2 = limbPts(p1, p2, 0.092f, 0.078f), a3 = limbPts(p2, p3, 0.078f, 0.064f);
        for (auto* q : {&a1, &a2, &a3}) r.drawConvex(grow(*q, OUT), INK, head, red);
        r.drawConvex(a1, KEFF, head, red); r.drawConvex(a2, KEFF, head, red); r.drawConvex(a3, KEFF_LITE, head, red);
        stroke(r, head, p0 + vec2(0.0f, -0.030f), p3 + vec2(0.0f, -0.022f), 0.012f, 0.010f, alpha(KEFF_DARK, 0.45f), red);
        stroke(r, head, p0 + vec2(0.0f,  0.030f), p3 + vec2(0.0f,  0.022f), 0.008f, 0.006f, alpha(KEFF_DARK, 0.30f), red);
        for (int k = 0; k < 3; ++k) {                                                                  // frayed end
            float yy = -0.020f + k * 0.020f;
            shape(r, head, {p3 + vec2(0.0f, yy + 0.012f), p3 + vec2(0.0f, yy - 0.012f),
                            p3 + vec2(-0.045f - (k % 2) * 0.020f, yy + flutter * 0.5f)}, k & 1 ? KEFF : KEFF_LITE, red, 0.003f);
        }

        vec2 q0(-0.085f, 0.800f), q1(-0.170f - back, 0.778f + flutter), q2(-0.235f - back * 1.4f, 0.718f + flutter * 1.6f);
        std::vector<vec2> b1 = limbPts(q0, q1, 0.080f, 0.064f), b2 = limbPts(q1, q2, 0.064f, 0.050f);
        for (auto* q : {&b1, &b2}) r.drawConvex(grow(*q, OUT), INK, head, red);
        r.drawConvex(b1, KEFF, head, red); r.drawConvex(b2, KEFF_LITE, head, red);
        vec2 d = safeDir(q2 - q1), n(-d.y, d.x);
        for (int k = 0; k < 2; ++k) {
            vec2 o = q2 + n * (-0.012f + k * 0.024f);
            shape(r, head, {o + n * 0.011f, o - n * 0.011f, o + d * 0.045f + vec2(0.0f, flutter * 0.4f)}, k ? KEFF : KEFF_LITE, red, 0.003f);
        }
    }

    // face (profile, looking right)
    shape(r, head, {{-0.040f, 0.935f}, {0.060f, 0.940f}, {0.108f, 0.905f}, {0.110f, 0.850f},
                    {0.095f, 0.800f}, {0.040f, 0.775f}, {-0.025f, 0.790f}, {-0.050f, 0.850f}}, SKIN, red);
    shape(r, head, {{-0.030f, 0.880f}, {-0.006f, 0.886f}, {-0.004f, 0.834f}, {-0.028f, 0.834f}}, SKIN_DARK, red, 0.003f);   // ear
    r.drawConvex({{-0.030f, 0.830f}, {0.020f, 0.848f}, {0.080f, 0.832f}, {0.098f, 0.815f}, {0.092f, 0.795f},
                  {0.040f, 0.772f}, {-0.020f, 0.785f}}, alpha(BEARD, 0.92f), head, red);                                // beard
    r.drawConvex({{-0.040f, 0.905f}, {0.000f, 0.905f}, {0.004f, 0.835f}, {-0.036f, 0.832f}}, alpha(BEARD, 0.85f), head, red);   // sideburn
    shape(r, head, {{0.050f, 0.866f}, {0.073f, 0.878f}, {0.092f, 0.866f}, {0.073f, 0.856f}}, WHITE, red, 0.003f);       // eye
    r.drawEllipse({0.078f, 0.866f}, vec2(0.0075f), BLACK, head, red, 10);
    r.drawConvex({{0.035f, 0.912f}, {0.108f, 0.890f}, {0.106f, 0.872f}, {0.040f, 0.893f}}, BEARD, head, red);           // scowling brow
    shape(r, head, {{0.100f, 0.878f}, {0.142f, 0.846f}, {0.096f, 0.836f}}, SKIN_LIGHT, red, 0.004f);                    // nose
    stroke(r, head, {0.074f, 0.820f}, {0.108f, 0.816f}, 0.006f, 0.006f, BLACK, red);                                    // set mouth

    // keffiyeh wrapped round the head, with a fine weave and two wrap bands
    {
        shape(r, head, {{-0.105f, 0.925f}, {-0.092f, 0.990f}, {-0.025f, 1.028f}, {0.060f, 1.025f},
                        {0.115f, 0.985f}, {0.124f, 0.940f}, {0.070f, 0.918f}, {-0.040f, 0.910f}}, KEFF, red);
        weave(r, head, {0.010f, 0.968f}, {0.105f, 0.052f}, 0.030f, alpha(KEFF_DARK, 0.32f), red);
        stroke(r, head, {-0.095f, 0.940f}, {0.118f, 0.945f}, 0.022f, 0.020f, alpha(KEFF_DARK, 0.50f), red);
        stroke(r, head, {-0.085f, 0.980f}, {0.095f, 1.005f}, 0.010f, 0.010f, alpha(KEFF_DARK, 0.30f), red);
        shape(r, head, {{-0.128f, 0.915f}, {-0.104f, 0.938f}, {-0.086f, 0.916f}, {-0.108f, 0.894f}}, KEFF_DARK, red, 0.004f);   // knot
    }

    // scarf round the neck + chin, draping to a V on the chest
    shape(r, head, {{-0.115f, 0.805f}, {0.095f, 0.800f}, {0.125f, 0.762f}, {0.100f, 0.700f},
                    {0.012f, 0.655f}, {-0.075f, 0.700f}, {-0.125f, 0.765f}}, KEFF, red);
    r.drawConvex({{-0.125f, 0.765f}, {-0.075f, 0.700f}, {-0.050f, 0.790f}}, alpha(ROBE_SHADE, 0.45f), head, red);
    weave(r, head, {0.000f, 0.745f}, {0.108f, 0.050f}, 0.030f, alpha(KEFF_DARK, 0.32f), red);
    stroke(r, head, {-0.100f, 0.790f}, {0.100f, 0.784f}, 0.014f, 0.012f, alpha(KEFF_DARK, 0.40f), red);
    for (int k = 0; k < 3; ++k) {                                                                      // fringe along the V
        float u = 0.15f + k * 0.30f;
        vec2 a = glm::mix(vec2(-0.075f, 0.700f), vec2(0.012f, 0.655f), u);
        r.drawTriangle(a + vec2(-0.011f, 0.0f), a + vec2(0.011f, 0.0f), a + vec2(-0.004f, -0.022f), KEFF_DARK, head, red);
        vec2 b = glm::mix(vec2(0.100f, 0.700f), vec2(0.012f, 0.655f), u);
        r.drawTriangle(b + vec2(-0.011f, 0.0f), b + vec2(0.011f, 0.0f), b + vec2(0.004f, -0.022f), KEFF_DARK, head, red);
    }

    // ------------------------------------------------ front arm: fully extended, aimed at the enemy
    {
        vec2 S(0.100f, 0.745f);
        vec2 H = glm::mix(vec2(0.270f, 0.600f), vec2(0.430f, 0.775f), aim);
        H += vec2(-0.025f * kick, 0.014f * kick);
        float ang = -26.0f * (1.0f - aim) + 14.0f * kick;
        vec2 W = H + vec2(-0.045f, 0.0f);
        vec2 E = ik(S, W, 0.155f, 0.150f, -1.0f);
        mat4 g = at(upper, H.x, H.y, ang);

        shape(r, upper, {S + vec2(-0.045f, 0.052f), S + vec2(0.040f, 0.052f), S + vec2(0.062f, -0.040f), S + vec2(-0.050f, -0.062f)}, ROBE, red);  // shoulder
        limb(r, upper, E - safeDir(W - E) * 0.04f, W, 0.078f, 0.066f, SKIN, red);          // bare forearm
        limb(r, upper, S, E, 0.100f, 0.090f, ROBE, red);                                   // sleeve
        band(r, upper, S, E, 0.92f, 0.104f, 0.044f, ROBE_LITE, red);                       // rolled cuff
        drawPistol(r, g, red);
        drawHand(r, g, red);
        band(r, upper, E, W, 0.90f, 0.078f, 0.032f, GLOVE, red);                           // glove wrist
        drawBang(r, upper, H, ang, t);

        // spent brass flicks out of the ejection port
        if (t > 0.02f && t < 0.40f) {
            vec2 e0 = H + rot(EJECT, ang);
            vec2 p = e0 + vec2(-0.10f * t, 0.24f * t - 1.1f * t * t);
            r.drawRect({0.0f, 0.0f}, {0.016f, 0.008f}, GOLD, at(upper, p.x, p.y, t * 900.0f), 0.0f);
        }
    }

    drawBullets(r);
    drawHealthBar(r, base);   // not tinted, not shaken
}

// ----------------------------------------------------------------- bullets (world space)
void MeleePlayer::drawBullets(Renderer2D& r) const
{
    for (const Bullet& b : bullets_) {
        float ang = (b.vx == 0.0f && b.vy == 0.0f) ? 0.0f : std::atan2(b.vy, b.vx);
        mat4 m = glm::scale(at(glm::translate(mat4(1.0f), glm::vec3(b.p, 0.0f)), 0.0f, 0.0f, glm::degrees(ang)),
                            glm::vec3(height_, height_, 1.0f));
        if (b.landed) {                                      // spent round lying in the dust, fading out
            float a = 1.0f - b.rest / BULLET_REST;
            r.drawConvex({{-0.030f, 0.006f}, {0.012f, 0.006f}, {0.024f, 0.0f}, {0.012f, -0.006f}, {-0.030f, -0.006f}},
                         alpha(GOLD, a), m, 0.0f);
            continue;
        }
        if (!b.dropping)                                     // tracer streak behind a flying bullet
            r.drawConvex({{-0.150f, 0.0f}, {-0.030f, 0.007f}, {-0.030f, -0.007f}}, alpha(FIRE, 0.45f), m, 0.0f);
        shape(r, m, {{-0.030f, 0.008f}, {0.010f, 0.008f}, {0.026f, 0.0f}, {0.010f, -0.008f}, {-0.030f, -0.008f}},
              GOLD, 0.0f, 0.003f);
        r.drawConvex({{0.008f, 0.007f}, {0.025f, 0.0f}, {0.008f, -0.007f}}, FIRE, m, 0.0f);   // copper tip
    }
}

// ----------------------------------------------------------------- health bar
void MeleePlayer::drawHealthBar(Renderer2D& r, const mat4& m) const
{
    const int   n = MAX_HEALTH;
    const float segW = 0.11f, gap = 0.016f;
    const float total = n * segW + (n - 1) * gap;
    const float cx = 0.0f, cy = 1.14f + jumpOffset();

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
