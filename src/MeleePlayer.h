#pragma once
#include "Renderer2D.h"
#include <glm/glm.hpp>

// Melee player: an Ancient Egyptian temple guard in linen wraps and blue-and-gold regalia,
// wielding paired bronze khopesh blades. The attack animation and short-range strike effect
// play locally; the game checks the blade's short horizontal arc with attackHits().
//
// Advances to the right on its own, attacks on a fixed rhythm (starting right after
// placement), jumps / ducks on command, 5 lives, 1 damage per strike.
//
// MODEL SPACE: origin at the feet (centre), +x = facing direction (right),
// +y = up, character is about 1.0 unit tall (nemes headcloth included). draw() scales that to
// 'height' world units, so the same code works for any lane size.
class MeleePlayer {
public:
    static constexpr int MAX_HEALTH = 5;
    static constexpr int DAMAGE     = 1;     // damage one close-range strike deals

    // feetPos: world position of the feet, height: character height in world units
    MeleePlayer(glm::vec2 feetPos, float height);

    void update(float dt);   // walking, khopesh attack cycle, jump/duck, hit timers
    void jump();             // W / Up    (ignored while already jumping or ducking)
    void duck();             // S / Down
    void takeHit(int damage = 1); // call once per enemy attack that reaches him
    void reset();

    // Call with true while he is touching an enemy: he stops walking but keeps attacking.
    void setEngaged(bool e)               { engaged_ = e; }
    void setSpeed(float worldUnitsPerSec) { speed_ = worldUnitsPerSec; }

    // Returns true once per attack cycle when the front khopesh strike begins. Use it for sound
    // effects etc. It does NOT mean anyone was hit - see attackHits().
    bool  consumeAttack();

    // Call once per enemy in this player's lane, every frame, with the enemy's body x-range (world units):
    //     if (p.attackHits(enemy.left, enemy.right)) enemy.hp -= MeleePlayer::DAMAGE;
    // True once during the swing when the khopesh arc overlaps the enemy.
    bool  attackHits(float xMin, float xMax); // true once per strike when an enemy overlaps the khopesh arc
    float reach() const { return 0.82f * height_; }   // khopesh tip in front of the feet

    bool isAlive()   const { return hp_ > 0; }
    bool isJumping() const { return jumpT_ > 0.0f; }
    bool isDucking() const { return duckT_ > 0.0f; }
    int  health()    const { return hp_; }
    glm::vec2 position() const { return pos_; }          // feet, world units
    float height()    const { return height_; }
    float halfWidth() const { return 0.16f * height_; }  // for body collision

    void draw(Renderer& r) const;         // character model
    void drawEffects(Renderer& r) const;  // strike effects and health bar

private:
    float jumpOffset() const;   // model units
    float duckAmount() const;   // 0..1
    float aimAmount() const;    // 0 = blade lowered, 1 = blade raised for a strike
    float sinceStrike() const;  // seconds since the current strike began
    void  drawHealthBar(Renderer2D& r, const glm::mat4& m) const;

    glm::vec2 pos_;
    float height_;
    int   hp_        = MAX_HEALTH;
    float time_      = 0.0f;    // drives breathing / nemes cloth sway
    float phase_     = 0.0f;    // walk cycle
    float walkBlend_ = 0.0f;    // 0 = standing stance, 1 = walking
    float hitTimer_  = 0.0f;
    float cycle_     = 0.10f;   // position in the attack cycle (first strike soon after placement)
    bool  fired_     = false;
    bool  strikeConsumed_ = false;
    bool  pendingAttack_ = false;
    bool  engaged_   = false;
    float jumpT_ = 0.0f, duckT_ = 0.0f;
    float speed_;
};
