#pragma once
#include "Renderer2D.h"
#include <glm/glm.hpp>

// Defense player: an Egyptian temple guardian in carved stone-and-gold armor with a shield.
// Stationary, deals no damage, absorbs 7 hits, ignores jump/duck.
//
// MODEL SPACE: origin at the feet (centre), +x = facing direction (right),
// +y = up, character is about 1.0 unit tall. draw() scales that to 'height'
// world units, so the same code works for any lane size.
class DefensePlayer {
public:
    static constexpr int MAX_HEALTH = 7;

    // feetPos: world position of the feet, height: character height in world units
    DefensePlayer(glm::vec2 feetPos, float height);

    void update(float dt);   // idle animation + hit-reaction timers
    void takeHit(int damage = 1); // call once per enemy attack that reaches him
    void reset();

    bool isAlive() const { return hp_ > 0; }
    int  health()  const { return hp_; }
    glm::vec2 position() const { return pos_; }
    float height() const { return height_; }

    void draw(Renderer& r) const;
    void drawEffects(Renderer& r) const;

private:
    void drawLegs(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawBackArm(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawTorso(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawLoincloth(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawHead(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawFrontUpperArm(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawShield(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawFrontForearm(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawHealthBar(Renderer2D& r, const glm::mat4& m) const;
    void drawKilt(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawCollar(Renderer2D& r, const glm::mat4& m, float red) const;
    void drawFrontArm(Renderer2D& r, const glm::mat4& m, float red) const;

    glm::vec2 pos_;
    float height_;
    int   hp_   = MAX_HEALTH;
    float time_ = 0.0f;       // drives the idle breathing
    float hitTimer_ = 0.0f;   // > 0 while reacting to a hit
};
