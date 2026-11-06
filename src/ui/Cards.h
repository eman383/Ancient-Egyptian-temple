#pragma once

#include "DefensePlayer.h"
#include "LongRangePlayer.h"
#include "MeleePlayer.h"
#include "PlayerManager.h"
#include "core/Game.h"
#include "game/Economy.h"

#include <functional>

// Player cards + coin counter, drawn in the top UI strip (y 750..900).
//
//  * 3 cards, one per player type. A card is UNLOCKED while coins >= its cost, LOCKED otherwise.
//    This is re-evaluated every frame, so cards unlock when coins are collected and lock again
//    when coins drop below the cost.
//  * Locked cards are drawn in grayscale and ignore the mouse completely (no hover, no click).
//  * Unlocked cards are drawn in colour, light up on hover, and can be clicked.
//  * Clicking an unlocked card selects that player type and places one on the selected lane
//    (exactly one lane must be selected, otherwise nothing is placed and the card flashes red).
//    The cost is charged only when a player was really placed.
//  * The coin counter is drawn at the top of the screen and always shows economy.coins.
//  * DEFENSE is placed in two steps: select one lane and click the Defense card, then
//    left-click ANY spot inside that lane to drop the shield there (a preview follows
//    the mouse). Right-click or clicking the Defense card again cancels. The cost is
//    charged only when the shield is really placed.

namespace cards {
constexpr int   COUNT  = 3;
constexpr float W      = 150.0f;
constexpr float H      = 125.0f;
constexpr float GAP    = 20.0f;
constexpr float X0     = 230.0f;   // left edge of the first card
constexpr float Y      = 765.0f;   // bottom edge of the cards
constexpr float ICON_H = 72.0f;    // height of the player model shown on a card

inline float left(int i)             { return X0 + i * (W + GAP); }
inline glm::vec2 iconFeet(int i)     { return {left(i) + W * 0.5f, Y + 34.0f}; }

// Cost of each player type (page 9: Long Range = 2x Melee, Defense = 3x Melee).
// The game starts with 30 coins = 3 Melee. Long Range / Defense also stay locked until
// 20 / 30 coins have been COLLECTED from enemies (see Economy::canAfford).
inline int cost(PlayerKind kind) {
    switch (kind) {
        case PlayerKind::Melee:     return 10;
        case PlayerKind::LongRange: return 20;   // 2x Melee (page 9)
        case PlayerKind::Defense:   return 30;   // 3x Melee (page 9)
    }
    return 0;
}
}

class CardsUI {
public:
    CardsUI();

    // Call once before Game::init()/run(), after PlayerManager::setup().
    void setup(Game& game, PlayerManager& players, Economy& economy);

    bool isUnlocked(int card) const;

    // Furthest x the shield may be dropped at in a lane (e.g. just in front of the
    // nearest enemy, so a shield is never placed behind or inside an enemy).
    void setPlacementLimit(std::function<float(int lane)> fn) { placementLimit_ = std::move(fn); }

private:
    void update(Game& game, float dt);
    void draw(Renderer& r) const;
    void drawCard(Renderer& r, int card) const;
    void drawCoinCounter(Renderer& r) const;
    void onCardClicked(Game& game, int card);
    void cancelShield();
    float shieldX(float mouseX) const;   // clamped drop position inside the lane
    void drawShieldPreview(Renderer& r) const;

    static PlayerKind kindOf(int card);

    PlayerManager* players_ = nullptr;
    Economy* economy_ = nullptr;

    int   hovered_ = -1;                 // unlocked card under the mouse, or -1
    float flash_[cards::COUNT] = {0, 0, 0};   // > 0: card just refused a placement (red border)

    // Shield placement mode (step 2): waiting for a click inside shieldLane_.
    bool  placingShield_ = false;
    int   shieldLane_ = -1;
    glm::vec2 mouse_{0.0f};
    std::function<float(int)> placementLimit_;

    // Models shown on the cards. Never updated, so they stay in their standing pose.
    MeleePlayer     melee_;
    DefensePlayer   defense_;
    LongRangePlayer longRange_;
};
