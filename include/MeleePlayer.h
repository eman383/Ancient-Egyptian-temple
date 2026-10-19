#pragma once
#include "Renderer2D.h"
#include <glm/glm.hpp>
#include <vector>

// Melee player: the "Galabeya Fighter" - a desert gunslinger in a dusty cream galabeya, a checkered
// keffiyeh and combat boots, with a bandolier rig and a pair of 1911 pistols. His weapons are used at
// CLOSE RANGE (point-blank): the shots animate in his hands (raise, bang, recoil, smoke, brass) and only
// BULLET: each volley sends a bullet out of the front pistol. It flies a short, fixed distance
// (close range); an enemy it touches in that distance takes damage, otherwise the bullet loses its
// momentum, drops to the ground and fades out. The game asks bulletHits() every frame per enemy.
//
// Advances to the right on its own, attacks on a fixed rhythm (starting right after
// placement), jumps / ducks on command, 5 lives, 1 damage per volley (the front pistol is the one that
// "counts"; the second pistol fires a beat later purely as animation).
//
// MODEL SPACE: origin at the feet (centre), +x = facing direction (right),
// +y = up, character is about 1.0 unit tall (keffiyeh included). draw() scales that to
// 'height' world units, so the same code works for any lane size.
class MeleePlayer {
public:
    static constexpr int MAX_HEALTH = 5;
    static constexpr int DAMAGE     = 1;     // damage one bullet deals
    static constexpr float BULLET_RANGE = 0.55f;   // flight distance after the muzzle, in character heights

    // feetPos: world position of the feet, height: character height in world units
    MeleePlayer(glm::vec2 feetPos, float height);

    void update(float dt);   // walking, shoot cycle, jump/duck, hit timers
    void jump();             // W / Up    (ignored while already jumping or ducking)
    void duck();             // S / Down
    void takeHit();          // call once per enemy attack that reaches him
    void reset();

    // Call with true while he is touching an enemy: he stops walking but keeps shooting.
    void setEngaged(bool e)               { engaged_ = e; }
    void setSpeed(float worldUnitsPerSec) { speed_ = worldUnitsPerSec; }

    // Returns true ONCE per volley, at the instant the front pistol goes off (a bullet has just been
    // spawned). Use it for sound effects etc. It does NOT mean anyone was hit - see bulletHits().
    bool  consumeAttack();

    // Call once per enemy in this player's lane, every frame, with the enemy's body x-range (world units):
    //     if (p.bulletHits(enemy.left, enemy.right)) enemy.hp -= MeleePlayer::DAMAGE;
    // True if a bullet that is still FLYING touched that range since the last update; that bullet is
    // used up (removed). Bullets that already dropped to the ground can't hit anything.
    bool  bulletHits(float xMin, float xMax);
    float reach() const { return 0.62f * height_; }   // where the muzzle is, in front of his feet
    float bulletRange() const { return BULLET_RANGE * height_; }   // how far a bullet flies after the muzzle

    bool isAlive()   const { return hp_ > 0; }
    bool isJumping() const { return jumpT_ > 0.0f; }
    bool isDucking() const { return duckT_ > 0.0f; }
    int  health()    const { return hp_; }
    glm::vec2 position() const { return pos_; }          // feet, world units
    float height()    const { return height_; }
    float halfWidth() const { return 0.16f * height_; }  // for body collision

    void draw(Renderer2D& r) const;   // body + health bar (nothing once defeated)

private:
    float jumpOffset() const;   // model units
    float duckAmount() const;   // 0..1
    float aimAmount() const;    // 0 = gun low, 1 = aimed at the enemy
    float sinceShot() const;    // seconds since the last bang (< 0 before it this cycle)
    void  drawHealthBar(Renderer2D& r, const glm::mat4& m) const;
    void  drawBullets(Renderer2D& r) const;
    void  spawnBullet();
    void  updateBullets(float dt);

    struct Bullet {
        glm::vec2 p, prev;          // world position now / at the previous update
        float vx, vy;               // world units per second
        float travelled = 0.0f;     // distance flown since leaving the muzzle
        float rest      = 0.0f;     // time spent lying on the ground
        bool  dropping  = false;    // out of range: falling
        bool  landed    = false;
    };
    std::vector<Bullet> bullets_;

    glm::vec2 pos_;
    float height_;
    int   hp_        = MAX_HEALTH;
    float time_      = 0.0f;    // drives breathing / bandana sway
    float phase_     = 0.0f;    // walk cycle
    float walkBlend_ = 0.0f;    // 0 = standing stance, 1 = walking
    float hitTimer_  = 0.0f;
    float cycle_     = 0.10f;   // position in the shoot cycle (first shot almost at once)
    bool  fired_     = false;
    bool  pendingAttack_ = false;
    bool  engaged_   = false;
    float jumpT_ = 0.0f, duckT_ = 0.0f;
    float speed_;
};
