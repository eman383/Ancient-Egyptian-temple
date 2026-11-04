#pragma once
#include "Renderer2D.h"
#include <glm/glm.hpp>
#include <vector>

// One arrow in flight. 'pos' is the world position of the arrow's CENTRE.
struct Arrow {
    glm::vec2 pos;
    glm::vec2 previous;
    bool  hit     = false;   // true once it struck an enemy (plays impact, then vanishes)
    float hitTime = 0.0f;
};

// Long-range player: an Egyptian archer. Her ONLY weapon is a bow.
// Advances to the right on its own, fires an arrow on a fixed rhythm, jumps / ducks
// on command, 5 lives, 1 damage per arrow.
//
// MODEL SPACE: origin at the feet (centre), +x = facing direction (right),
// +y = up, character is about 1.0 unit tall. draw() scales that to 'height'
// world units, so the same code works for any lane size.
class LongRangePlayer {
public:
    static constexpr int MAX_HEALTH = 5;
    static constexpr int DAMAGE     = 1;     // damage one arrow deals to an enemy

    // feetPos: world position of the feet, height: character height in world units
    LongRangePlayer(glm::vec2 feetPos, float height);

    void update(float dt);   // walking, bow cycle, arrows, jump/duck, hit timers
    void jump();             // W / Up    (ignored while already jumping or ducking)
    void duck();             // S / Down
    void takeHit(int damage = 1); // call once per enemy attack that reaches her
    void reset();

    // Call with true while she is touching an enemy: she stops walking but keeps shooting.
    void setEngaged(bool e)                    { engaged_ = e; }
    void setSpeed(float worldUnitsPerSec)      { speed_ = worldUnitsPerSec; }
    void setArrowSpeed(float worldUnitsPerSec) { arrowSpeed_ = worldUnitsPerSec; }
    void setArrowMaxX(float x)                 { arrowMaxX_ = x; }  // arrows past this x are removed

    bool isAlive()   const { return hp_ > 0; }
    bool isJumping() const { return jumpT_ > 0.0f; }
    bool isDucking() const { return duckT_ > 0.0f; }
    int  health()    const { return hp_; }
    glm::vec2 position() const { return pos_; }          // feet, world units
    float height()    const { return height_; }
    float halfWidth() const { return 0.16f * height_; }  // for body collision

    // ---- arrows (the game does the collision test against enemies) ----
    const std::vector<Arrow>& arrows() const { return arrows_; }
    glm::vec2 arrowTip(const Arrow& a) const { return a.pos + glm::vec2(0.275f * height_, 0.0f); }
    void arrowHit(size_t index);   // call when arrow 'index' touches an enemy
    bool arrowHitsEnemy(size_t index, float enemyXMin, float enemyXMax);

    // True when she is dead AND all her arrows are gone -> safe to delete the object.
    bool isRemovable() const { return hp_ <= 0 && arrows_.empty(); }

    void draw(Renderer& r) const;         // character model
    void drawEffects(Renderer& r) const;  // arrows and health bar

private:
    float jumpOffset() const;   // model units
    float duckAmount() const;   // 0..1
    float drawAmount() const;   // 0..1 how far the string is pulled
    float vibration() const;    // string wobble right after release
    void  fire();

    void drawArrow(Renderer2D& r, const Arrow& a) const;
    void drawHealthBar(Renderer2D& r, const glm::mat4& m) const;

    glm::vec2 pos_;
    float height_;
    int   hp_        = MAX_HEALTH;
    float time_      = 0.0f;    // drives breathing / hair sway
    float phase_     = 0.0f;    // walk cycle
    float walkBlend_ = 0.0f;    // 0 = standing stance, 1 = walking
    float hitTimer_  = 0.0f;
    float cycle_     = 0.35f;   // position in the shoot cycle (starts almost at once)
    bool  fired_     = false;
    bool  engaged_   = false;
    float jumpT_ = 0.0f, duckT_ = 0.0f;
    float speed_, arrowSpeed_;
    float arrowMaxX_ = 1.0e9f;
    std::vector<Arrow> arrows_;
};
