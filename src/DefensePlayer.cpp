#include "DefensePlayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

using glm::vec2;
using glm::vec4;
using glm::mat4;

namespace {
// ---- Palette (matched to the reference art) ----
const vec4 STONE        (0.60f, 0.53f, 0.44f, 1.0f);
const vec4 STONE_LIGHT  (0.72f, 0.65f, 0.54f, 1.0f);
const vec4 STONE_SHADE  (0.46f, 0.40f, 0.33f, 1.0f);
const vec4 STONE_DARK   (0.34f, 0.29f, 0.24f, 1.0f);
const vec4 STONE_HEAD   (0.58f, 0.52f, 0.44f, 1.0f);
const vec4 OUTLINE      (0.16f, 0.12f, 0.09f, 1.0f);
const vec4 GOLD         (0.80f, 0.62f, 0.26f, 1.0f);
const vec4 GOLD_LIGHT   (0.92f, 0.78f, 0.42f, 1.0f);
const vec4 GOLD_DARK    (0.50f, 0.36f, 0.14f, 1.0f);
const vec4 BLUE         (0.30f, 0.38f, 0.64f, 1.0f);
const vec4 BLUE_DARK    (0.20f, 0.26f, 0.50f, 1.0f);
const vec4 LINEN        (0.82f, 0.75f, 0.58f, 1.0f);
const vec4 LINEN_SHADE  (0.66f, 0.60f, 0.46f, 1.0f);
const vec4 SHIELD_PANEL (0.52f, 0.47f, 0.40f, 1.0f);
const vec4 SHIELD_SIDE  (0.38f, 0.28f, 0.13f, 1.0f);

const float HIT_TIME = 0.35f;   // seconds of shake/flash after a hit

mat4 at(const mat4& m, float x, float y, float deg = 0.0f)
{
    mat4 r = glm::translate(m, glm::vec3(x, y, 0.0f));
    if (deg != 0.0f) r = glm::rotate(r, glm::radians(deg), glm::vec3(0, 0, 1));
    return r;
}

// Small helper so the drawing code below stays short and readable.
struct Brush {
    Renderer2D& r;
    const mat4& m;
    float red;

    void poly(const std::vector<vec2>& p, const vec4& c) const { r.drawConvex(p, c, m, red); }
    void tri(vec2 a, vec2 b, vec2 c, const vec4& col) const { r.drawTriangle(a, b, c, col, m, red); }
    // low 'seg' values give the faceted, carved-stone look
    void ell(vec2 c, vec2 rad, const vec4& col, int seg = 28) const { r.drawEllipse(c, rad, col, m, red, seg); }
    void rect(vec2 c, vec2 s, const vec4& col, float deg = 0.0f) const
    {
        r.drawRect({0.0f, 0.0f}, s, col, at(m, c.x, c.y, deg), red);
    }
    // tapered limb with a dark outline
    void limb(vec2 a, vec2 b, float wa, float wb, const vec4& col, bool outline = true) const
    {
        if (outline) r.drawLimb(a, b, wa + 0.012f, wb + 0.012f, OUTLINE, m, red);
        r.drawLimb(a, b, wa, wb, col, m, red);
    }
    static std::vector<vec2> scaled(const std::vector<vec2>& p, float sx, float sy)
    {
        vec2 c(0.0f);
        for (const vec2& v : p) c += v;
        c /= (float)p.size();
        std::vector<vec2> q;
        for (const vec2& v : p) q.push_back(c + vec2((v.x - c.x) * sx, (v.y - c.y) * sy));
        return q;
    }
    // convex polygon with a dark outline (outline = same shape, slightly bigger)
    void outlined(const std::vector<vec2>& p, const vec4& col, float grow = 1.05f) const
    {
        poly(scaled(p, grow, grow), OUTLINE);
        poly(p, col);
    }
    // lower half of an ellipse (flat edge on top) - used for the collar
    void halfDisc(vec2 c, float rx, float ry, const vec4& col, int seg = 32) const
    {
        std::vector<vec2> p;
        p.push_back(c);
        for (int i = 0; i <= seg; ++i) {
            float a = 3.14159265f + 3.14159265f * (float)i / (float)seg;
            p.push_back(c + vec2(std::cos(a) * rx, std::sin(a) * ry));
        }
        poly(p, col);
    }
    // a row of tiny carved "hieroglyph" blocks
    void glyphs(vec2 start, vec2 step, int n, const vec4& col) const
    {
        static const float sw[3] = {0.014f, 0.008f, 0.016f};
        static const float sh[3] = {0.016f, 0.018f, 0.009f};
        for (int i = 0; i < n; ++i)
            rect(start + step * (float)i, {sw[i % 3], sh[i % 3]}, col);
    }
};
} // namespace

DefensePlayer::DefensePlayer(vec2 feetPos, float height)
    : pos_(feetPos), height_(height) {}

void DefensePlayer::update(float dt)
{
    time_ += dt;
    if (hitTimer_ > 0.0f) hitTimer_ = std::max(0.0f, hitTimer_ - dt);
}

void DefensePlayer::takeHit()
{
    if (hp_ <= 0) return;
    --hp_;
    hitTimer_ = HIT_TIME;
}

void DefensePlayer::reset()
{
    hp_ = MAX_HEALTH;
    hitTimer_ = 0.0f;
}

// ------------------------------------------------------------------
// Hierarchy:  base (world position + scale)
//               root  (+ hit shake)           legs, kilt
//                 upper (+ breathing bob)     torso, head, arms
//                   shield (+ recoil on hit)
// ------------------------------------------------------------------
void DefensePlayer::draw(Renderer2D& r) const
{
    if (hp_ <= 0) return;   // defeated -> removed from the scene

    float flash = hitTimer_ / HIT_TIME;                       // 1 -> 0 after a hit
    float shake = std::sin(hitTimer_ * 70.0f) * 0.015f * flash;
    float bob   = std::sin(time_ * 2.2f) * 0.006f;

    // Damage tint: grows as health drops, plus a short flash on each hit
    float lost = 1.0f - (float)hp_ / (float)MAX_HEALTH;
    float red  = std::min(1.0f, lost * 0.75f + flash * 0.25f);

    mat4 base  = glm::scale(glm::translate(mat4(1.0f), glm::vec3(pos_, 0.0f)),
                            glm::vec3(height_, height_, 1.0f));
    mat4 root  = at(base, shake, 0.0f);
    mat4 upper = at(root, 0.0f, bob);

    drawLegs(r, root, red);
    drawBackArm(r, upper, red);
    drawTorso(r, upper, red);
    drawKilt(r, root, red);
    drawCollar(r, upper, red);
    drawHead(r, upper, red);
    drawFrontArm(r, upper, red);
    drawShield(r, at(upper, -0.03f * flash, 0.0f), red);     // recoils when hit

    drawHealthBar(r, base);   // not tinted, not shaken
}

// ---------------------------------------------------------------- legs
void DefensePlayer::drawLegs(Renderer2D& r, const mat4& m, float red) const
{
    Brush b{r, m, red};

    // back leg (screen left) and front leg: thick, angular stone
    b.limb({-0.050f, 0.340f}, {-0.080f, 0.165f}, 0.125f, 0.105f, STONE);
    b.limb({-0.080f, 0.165f}, {-0.105f, 0.050f}, 0.105f, 0.075f, STONE);
    b.limb({ 0.070f, 0.340f}, { 0.105f, 0.160f}, 0.130f, 0.105f, STONE);
    b.limb({ 0.105f, 0.160f}, { 0.135f, 0.050f}, 0.105f, 0.075f, STONE);

    // carved muscle facets: low-segment ellipses read as cut stone
    b.ell({-0.082f, 0.170f}, {0.046f, 0.040f}, STONE_LIGHT, 6);       // knees
    b.ell({ 0.106f, 0.165f}, {0.046f, 0.040f}, STONE_LIGHT, 6);
    b.ell({-0.062f, 0.275f}, {0.040f, 0.050f}, STONE_LIGHT, 5);       // thighs
    b.ell({ 0.090f, 0.275f}, {0.042f, 0.050f}, STONE_LIGHT, 5);
    b.ell({-0.098f, 0.105f}, {0.030f, 0.050f}, STONE_SHADE, 5);       // calves
    b.ell({ 0.128f, 0.105f}, {0.030f, 0.050f}, STONE_SHADE, 5);
    b.poly({{-0.040f, 0.320f}, {-0.012f, 0.300f}, {-0.030f, 0.230f}, {-0.058f, 0.250f}}, STONE_SHADE);

    // bare feet, toes pointing the way he faces (right)
    b.ell({-0.060f, 0.030f}, {0.078f, 0.034f}, OUTLINE);
    b.ell({-0.060f, 0.031f}, {0.070f, 0.028f}, STONE);
    b.ell({ 0.185f, 0.030f}, {0.076f, 0.034f}, OUTLINE);
    b.ell({ 0.185f, 0.031f}, {0.068f, 0.028f}, STONE);
    const float toeX[4] = {0.066f, 0.074f, 0.076f, 0.070f};
    const float toeY[4] = {0.046f, 0.036f, 0.026f, 0.016f};
    for (int i = 0; i < 4; ++i) {
        b.ell({-0.060f + toeX[i], toeY[i]}, {0.012f, 0.0085f}, STONE_LIGHT, 8);
        b.ell({ 0.185f + toeX[i], toeY[i]}, {0.012f, 0.0085f}, STONE_LIGHT, 8);
    }
}

// ------------------------------------------- back (left-hand) arm + fist
void DefensePlayer::drawBackArm(Renderer2D& r, const mat4& m, float red) const
{
    Brush b{r, m, red};

    b.limb({-0.160f, 0.685f}, {-0.205f, 0.555f}, 0.145f, 0.125f, STONE);    // upper arm
    b.limb({-0.205f, 0.555f}, {-0.150f, 0.425f}, 0.130f, 0.105f, STONE);    // forearm
    b.ell({-0.185f, 0.600f}, {0.040f, 0.050f}, STONE_LIGHT, 6);

    // wide gold armband with carved glyphs
    b.limb({-0.168f, 0.668f}, {-0.182f, 0.615f}, 0.158f, 0.150f, GOLD);
    b.rect({-0.164f, 0.672f}, {0.150f, 0.008f}, GOLD_DARK, -15.0f);
    b.rect({-0.184f, 0.612f}, {0.145f, 0.008f}, GOLD_DARK, -15.0f);
    b.glyphs({-0.208f, 0.640f}, {0.0165f, 0.0f}, 5, GOLD_DARK);

    // gold bracer on the forearm with a zig-zag edge
    b.limb({-0.200f, 0.540f}, {-0.168f, 0.462f}, 0.145f, 0.128f, GOLD);
    b.rect({-0.202f, 0.546f}, {0.140f, 0.008f}, GOLD_DARK, 23.0f);
    b.rect({-0.166f, 0.458f}, {0.125f, 0.008f}, GOLD_DARK, 23.0f);
    for (int i = 0; i < 4; ++i) {
        float t = (float)i / 3.0f;
        vec2 p = glm::mix(vec2(-0.215f, 0.520f), vec2(-0.172f, 0.485f), t);
        b.tri(p + vec2(0.0f, 0.016f), p + vec2(0.014f, -0.010f), p + vec2(-0.014f, -0.010f), GOLD_DARK);
    }

    // clenched fist
    b.ell({-0.142f, 0.385f}, {0.062f, 0.056f}, OUTLINE, 10);
    b.ell({-0.142f, 0.385f}, {0.055f, 0.049f}, STONE, 10);
    for (int i = 0; i < 3; ++i)
        b.rect({-0.142f, 0.402f - i * 0.018f}, {0.100f, 0.004f}, STONE_DARK);
    b.ell({-0.108f, 0.378f}, {0.018f, 0.030f}, STONE_LIGHT, 6);
}

// ---------------------------------------------------------------- torso
void DefensePlayer::drawTorso(Renderer2D& r, const mat4& m, float red) const
{
    Brush b{r, m, red};

    b.rect({0.030f, 0.775f}, {0.070f, 0.060f}, STONE_SHADE);          // neck

    // broad chest tapering to the waist
    b.outlined({{-0.172f, 0.700f}, {-0.078f, 0.770f}, {0.103f, 0.770f},
                { 0.164f, 0.700f}, { 0.121f, 0.520f}, {-0.078f, 0.520f}}, STONE, 1.02f);

    // shaded right flank for volume
    b.poly({{0.100f, 0.700f}, {0.164f, 0.700f}, {0.121f, 0.520f}, {0.078f, 0.520f}}, STONE_SHADE);

    // pecs (carved facets)
    b.poly({{-0.135f, 0.700f}, {-0.010f, 0.715f}, {-0.005f, 0.622f}, {-0.118f, 0.638f}}, STONE_LIGHT);
    b.poly({{ 0.010f, 0.715f}, { 0.112f, 0.700f}, { 0.122f, 0.640f}, { 0.010f, 0.622f}}, STONE_LIGHT);
    b.rect({0.002f, 0.665f}, {0.008f, 0.100f}, STONE_DARK);

    // abs: two rows of blocks
    for (int i = 0; i < 2; ++i) {
        float y = 0.592f - i * 0.046f;
        b.poly({{-0.066f, y + 0.017f}, {-0.008f, y + 0.017f}, {-0.008f, y - 0.017f}, {-0.062f, y - 0.017f}}, STONE_LIGHT);
        b.poly({{ 0.012f, y + 0.017f}, { 0.068f, y + 0.017f}, { 0.064f, y - 0.017f}, { 0.012f, y - 0.017f}}, STONE_LIGHT);
    }

    // carved hieroglyph band across the upper chest
    b.glyphs({-0.118f, 0.686f}, {0.0215f, 0.0f}, 11, STONE_DARK);
    b.glyphs({-0.100f, 0.654f}, {0.0215f, 0.0f}, 9, STONE_DARK);

    // shoulders (faceted balls)
    b.ell({-0.152f, 0.690f}, {0.086f, 0.082f}, OUTLINE, 9);
    b.ell({-0.152f, 0.690f}, {0.078f, 0.074f}, STONE, 9);
    b.poly({{-0.205f, 0.715f}, {-0.150f, 0.745f}, {-0.110f, 0.715f}, {-0.160f, 0.690f}}, STONE_LIGHT);
    b.ell({ 0.146f, 0.690f}, {0.082f, 0.080f}, OUTLINE, 9);
    b.ell({ 0.146f, 0.690f}, {0.074f, 0.072f}, STONE_SHADE, 9);
}

// ----------------------------------------------------------- kilt + belt
void DefensePlayer::drawKilt(Renderer2D& r, const mat4& m, float red) const
{
    Brush b{r, m, red};

    const vec2 tl(-0.080f, 0.475f), tr(0.150f, 0.475f), bl(-0.088f, 0.280f), br(0.165f, 0.280f);
    b.outlined({tl, tr, br, bl}, LINEN, 1.03f);

    // gold and blue stripes down the linen
    const int n = 8;
    for (int i = 0; i < n; ++i) {
        const vec4* col = nullptr;
        if (i == 1 || i == 5) col = &GOLD;
        else if (i == 3 || i == 7) col = &LINEN_SHADE;
        else if (i == 2)  col = &BLUE;
        if (!col) continue;
        float t0 = (float)i / n, t1 = (float)(i + 1) / n;
        b.poly({glm::mix(tl, tr, t0), glm::mix(tl, tr, t1), glm::mix(bl, br, t1), glm::mix(bl, br, t0)}, *col);
    }
    b.poly({{-0.088f, 0.280f}, {0.165f, 0.280f}, {0.164f, 0.298f}, {-0.087f, 0.298f}}, GOLD);

    // front apron: gold with a blue centre stripe
    b.outlined({{0.040f, 0.475f}, {0.150f, 0.475f}, {0.146f, 0.255f}, {0.098f, 0.222f}, {0.048f, 0.255f}},
               GOLD, 1.04f);
    b.poly({{0.082f, 0.475f}, {0.108f, 0.475f}, {0.106f, 0.240f}, {0.098f, 0.232f}, {0.088f, 0.240f}}, BLUE);
    b.rect({0.095f, 0.360f}, {0.004f, 0.220f}, BLUE_DARK);

    // gold belt with a trapezoid buckle
    b.outlined({{-0.082f, 0.522f}, {0.150f, 0.522f}, {0.150f, 0.472f}, {-0.082f, 0.472f}}, GOLD, 1.03f);
    b.rect({0.034f, 0.512f}, {0.226f, 0.005f}, GOLD_DARK);
    b.rect({0.034f, 0.482f}, {0.226f, 0.005f}, GOLD_DARK);
    b.outlined({{0.038f, 0.520f}, {0.122f, 0.520f}, {0.110f, 0.468f}, {0.050f, 0.468f}}, GOLD_LIGHT, 1.05f);
    b.poly({{0.052f, 0.508f}, {0.108f, 0.508f}, {0.100f, 0.480f}, {0.060f, 0.480f}}, GOLD);
}

// --------------------------------------------------------------- collar
// Wide gold-and-blue usekh necklace.
void DefensePlayer::drawCollar(Renderer2D& r, const mat4& m, float red) const
{
    Brush b{r, m, red};
    const vec2 c(0.030f, 0.776f);

    b.halfDisc(c, 0.128f, 0.084f, OUTLINE);
    b.halfDisc(c, 0.120f, 0.077f, GOLD);
    b.halfDisc(c, 0.102f, 0.063f, BLUE);
    b.halfDisc(c, 0.084f, 0.050f, GOLD_LIGHT);
    b.halfDisc(c, 0.066f, 0.038f, BLUE_DARK);
    b.halfDisc(c, 0.048f, 0.026f, GOLD);

    for (int i = 1; i < 10; ++i) {
        float a = 3.14159265f + 3.14159265f * (float)i / 10.0f;
        vec2 p = c + vec2(std::cos(a) * 0.111f, std::sin(a) * 0.070f);
        b.ell(p, {0.008f, 0.008f}, GOLD_LIGHT, 8);
    }
}

// ----------------------------------------------------------------- head
// Jackal (Anubis) head in profile, facing right, with a striped headdress.
void DefensePlayer::drawHead(Renderer2D& r, const mat4& m, float red) const
{
    // the head is drawn 20% larger than its raw numbers, scaled about the neck
    mat4 hm = glm::translate(m, glm::vec3(0.030f, 0.775f, 0.0f));
    hm = glm::scale(hm, glm::vec3(1.2f, 1.2f, 1.0f));
    hm = glm::translate(hm, glm::vec3(-0.030f, -0.775f, 0.0f));
    Brush b{r, hm, red};

    // tall ears: gold rim with a stone centre
    b.outlined({{-0.048f, 0.862f}, {0.006f, 1.010f}, {0.040f, 0.868f}}, GOLD, 1.06f);
    b.tri({-0.030f, 0.882f}, {0.006f, 0.975f}, {0.026f, 0.882f}, STONE_SHADE);
    b.outlined({{0.020f, 0.872f}, {0.080f, 0.990f}, {0.098f, 0.878f}}, GOLD, 1.06f);
    b.tri({0.040f, 0.890f}, {0.077f, 0.966f}, {0.084f, 0.890f}, STONE_HEAD);

    // striped lappet hanging behind the head
    const vec2 A(-0.060f, 0.880f), B(-0.112f, 0.735f), C(-0.022f, 0.735f), D(0.0f, 0.875f);
    b.poly({A + vec2(-0.008f, 0.006f), B + vec2(-0.012f, -0.008f), C + vec2(0.010f, -0.008f), D + vec2(0.006f, 0.006f)}, OUTLINE);
    b.poly({A, B, C, D}, GOLD);
    for (int i = 0; i < 8; i += 2) {
        float t0 = (float)i / 8.0f, t1 = (float)(i + 1) / 8.0f;
        b.poly({glm::mix(A, B, t0), glm::mix(A, B, t1), glm::mix(D, C, t1), glm::mix(D, C, t0)}, BLUE);
    }

    // skull and long snout
    b.ell({0.015f, 0.875f}, {0.066f, 0.073f}, OUTLINE);
    b.ell({0.015f, 0.875f}, {0.060f, 0.067f}, STONE_HEAD);
    b.outlined({{0.040f, 0.893f}, {0.108f, 0.846f}, {0.122f, 0.824f}, {0.092f, 0.801f}, {0.038f, 0.797f}},
               STONE_HEAD, 1.04f);
    b.poly({{0.060f, 0.880f}, {0.106f, 0.846f}, {0.112f, 0.834f}, {0.062f, 0.848f}}, STONE_LIGHT);   // snout ridge
    b.ell({0.117f, 0.826f}, {0.012f, 0.010f}, OUTLINE, 10);                                          // nose
    b.rect({0.078f, 0.808f}, {0.066f, 0.005f}, STONE_DARK, 6.0f);                                    // mouth

    // headdress cap: gold with blue stripes
    b.ell({0.008f, 0.906f}, {0.070f, 0.054f}, OUTLINE);
    b.ell({0.008f, 0.906f}, {0.064f, 0.048f}, GOLD);
    b.ell({0.004f, 0.904f}, {0.054f, 0.040f}, BLUE);
    for (int i = -1; i <= 1; ++i) {
        float dx = i * 0.028f;
        float h = 2.0f * 0.040f * std::sqrt(std::max(0.0f, 1.0f - (dx / 0.054f) * (dx / 0.054f)));
        b.rect({0.004f + dx, 0.904f}, {0.008f, h}, GOLD);
    }
    b.rect({0.012f, 0.873f}, {0.118f, 0.014f}, GOLD, -4.0f);                 // headband

    // gem on the headdress, brow and eye
    b.ell({0.000f, 0.862f}, {0.021f, 0.021f}, GOLD, 14);
    b.ell({0.000f, 0.862f}, {0.014f, 0.014f}, BLUE, 14);
    b.rect({0.058f, 0.883f}, {0.046f, 0.010f}, STONE_DARK, -14.0f);
    b.ell({0.058f, 0.864f}, {0.017f, 0.009f}, OUTLINE, 12);
    b.ell({0.059f, 0.864f}, {0.012f, 0.006f}, GOLD_LIGHT, 12);
}

// ------------------------------------------------------------ front arm
// The shield arm: only the shoulder and upper arm show; the rest is behind the shield.
void DefensePlayer::drawFrontArm(Renderer2D& r, const mat4& m, float red) const
{
    Brush b{r, m, red};
    b.limb({0.150f, 0.680f}, {0.150f, 0.560f}, 0.130f, 0.115f, STONE_SHADE);
}

// --------------------------------------------------------------- shield
// Tall bronze shield seen at an angle: gold rim with a chevron border,
// weathered stone panel, and two ankhs.
void DefensePlayer::drawShield(Renderer2D& r, const mat4& m, float red) const
{
    mat4 s = at(m, 0.222f, 0.450f, -8.0f);           // slight tilt like the reference
    Brush g{r, s, red};

    // elongated hexagon, wide in the middle and pointed at both ends
    const std::vector<vec2> hex = {{0.0f, 0.400f}, {0.092f, 0.270f}, {0.092f, -0.170f},
                                   {0.0f, -0.400f}, {-0.092f, -0.170f}, {-0.092f, 0.270f}};

    // visible thickness on the inner (left) side
    std::vector<vec2> side = hex;
    for (vec2& v : side) v.x -= 0.032f;
    g.poly(Brush::scaled(side, 1.05f, 1.02f), OUTLINE);
    g.poly(side, SHIELD_SIDE);

    g.poly(Brush::scaled(hex, 1.06f, 1.025f), OUTLINE);
    g.poly(hex, GOLD);                                                  // rim
    g.poly(Brush::scaled(hex, 0.80f, 0.93f), SHIELD_PANEL);             // panel

    // chevron border down both long edges
    for (int i = 0; i < 15; ++i) {
        float y = -0.165f + i * 0.030f;
        g.tri({ 0.090f, y}, { 0.090f, y + 0.030f}, { 0.077f, y + 0.015f}, GOLD_DARK);
        g.tri({-0.090f, y}, {-0.090f, y + 0.030f}, {-0.077f, y + 0.015f}, GOLD_DARK);
    }

    g.rect({0.0f, 0.0f}, {0.006f, 0.740f}, STONE_SHADE);                // centre ridge

    // weathering marks
    g.ell({-0.035f,  0.060f}, {0.020f, 0.014f}, STONE_SHADE, 7);
    g.ell({ 0.030f, -0.070f}, {0.018f, 0.020f}, STONE_SHADE, 7);
    g.ell({-0.025f, -0.320f}, {0.016f, 0.012f}, STONE_SHADE, 7);
    g.ell({ 0.035f,  0.300f}, {0.014f, 0.010f}, STONE_SHADE, 7);

    // ankhs: dark shadow pass first, then gold
    auto ankh = [&](float cx, float cy, float k) {
        for (int pass = 0; pass < 2; ++pass) {
            float e = pass == 0 ? 0.008f * k : 0.0f;
            const vec4& col = pass == 0 ? GOLD_DARK : GOLD_LIGHT;
            g.ell({cx, cy + 0.055f * k}, {0.028f * k + e, 0.036f * k + e}, col, 20);
            if (pass == 1) g.ell({cx, cy + 0.055f * k}, {0.015f * k, 0.021f * k}, SHIELD_PANEL, 16);
            g.rect({cx, cy}, {0.085f * k + 2 * e, 0.020f * k + 2 * e}, col);
            g.rect({cx, cy - 0.070f * k}, {0.022f * k + 2 * e, 0.150f * k + 2 * e}, col);
        }
    };
    ankh(0.0f,  0.140f, 1.00f);
    ankh(0.0f, -0.200f, 0.62f);

    // rivets at the shield's tips
    g.ell({0.0f,  0.350f}, {0.012f, 0.012f}, GOLD_LIGHT, 10);
    g.ell({0.0f, -0.350f}, {0.012f, 0.012f}, GOLD_LIGHT, 10);
}

// ----------------------------------------------------------- health bar
// 7 segments, one per hit he can absorb. Always visible above his head.
void DefensePlayer::drawHealthBar(Renderer2D& r, const mat4& m) const
{
    const int   n = MAX_HEALTH;
    const float segW = 0.060f, gap = 0.010f;
    const float total = n * segW + (n - 1) * gap;
    const float cx = 0.05f, cy = 1.12f;

    float frac = (float)hp_ / (float)n;
    vec4 fill = frac > 0.5f ? vec4(0.20f, 0.80f, 0.20f, 1.0f)
              : frac > 0.25f ? vec4(0.95f, 0.80f, 0.15f, 1.0f)
                             : vec4(0.90f, 0.15f, 0.15f, 1.0f);

    r.drawRect({cx, cy}, {total + 0.02f, 0.045f}, vec4(0.08f, 0.08f, 0.08f, 1.0f), m);
    for (int i = 0; i < n; ++i) {
        float x = cx - total * 0.5f + segW * 0.5f + i * (segW + gap);
        r.drawRect({x, cy}, {segW, 0.028f},
                   i < hp_ ? fill : vec4(0.28f, 0.28f, 0.28f, 1.0f), m);
    }
}
