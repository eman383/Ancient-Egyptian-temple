#pragma once

#include "DefensePlayer.h"
#include "LongRangePlayer.h"
#include "MeleePlayer.h"
#include "PlayerManager.h"
#include "core/Game.h"
#include "game/Economy.h"

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

// Cost of each player type. The game starts with 30 coins: only Melee is affordable.
inline int cost(PlayerKind kind) {
    switch (kind) {
        case PlayerKind::Melee:     return 10;
        case PlayerKind::LongRange: return 40;
        case PlayerKind::Defense:   return 50;
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

private:
    void update(Game& game, float dt);
    void draw(Renderer& r) const;
    void drawCard(Renderer& r, int card) const;
    void drawCoinCounter(Renderer& r) const;
    void onCardClicked(Game& game, int card);

    static PlayerKind kindOf(int card);

    PlayerManager* players_ = nullptr;
    Economy* economy_ = nullptr;

    int   hovered_ = -1;                 // unlocked card under the mouse, or -1
    float flash_[cards::COUNT] = {0, 0, 0};   // > 0: card just refused a placement (red border)

    // Models shown on the cards. Never updated, so they stay in their standing pose.
    MeleePlayer     melee_;
    DefensePlayer   defense_;
    LongRangePlayer longRange_;
};
