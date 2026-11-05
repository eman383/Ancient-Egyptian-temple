
#include "Economy.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

constexpr float CARD_Y = 770.0f;
constexpr float CARD_W = 145.0f;
constexpr float CARD_H = 111.0f;

float cardX(int i) {
    return 24.0f + 165.0f * i;
}

int cost(PlayerKind kind) {
    switch (kind) {
        case PlayerKind::Melee:
            return Economy::MELEE_COST;
        case PlayerKind::LongRange:
            return Economy::RANGED_COST;
        case PlayerKind::Defense:
            return Economy::DEFENSE_COST;
    }
    return 0;
}

PlayerKind typeAt(int i) {
    if (i == 0) return PlayerKind::Melee;
    if (i == 1) return PlayerKind::LongRange;
    return PlayerKind::Defense;
}

bool inside(
    glm::vec2 p, float x, float y, float w, float h
) {
    return p.x >= x && p.x < x + w &&
           p.y >= y && p.y < y + h;
}

// Draw numbers using seven segments because the
// existing Renderer has no text-drawing function.
void digit(Renderer& r, int n, float x, float y, glm::vec4 c) {
    static constexpr int masks[10] = {
        0x3f, 0x06, 0x5b, 0x4f, 0x66,
        0x6d, 0x7d, 0x07, 0x7f, 0x6f
    };

    if (n < 0 || n > 9) return;

    int m = masks[n];

    if (m & 1)  r.rectBL({x+3,y+24},{11,3},c);
    if (m & 2)  r.rectBL({x+14,y+14},{3,10},c);
    if (m & 4)  r.rectBL({x+14,y+3},{3,10},c);
    if (m & 8)  r.rectBL({x+3,y},{11,3},c);
    if (m & 16) r.rectBL({x,y+3},{3,10},c);
    if (m & 32) r.rectBL({x,y+14},{3,10},c);
    if (m & 64) r.rectBL({x+3,y+12},{11,3},c);
}

void number(
    Renderer& r, int value, float x, float y, glm::vec4 color
) {
    std::string s = std::to_string(std::max(0, value));

    for (std::size_t i = 0; i < s.size(); ++i) {
        digit(r, s[i] - '0',
              x + 21.0f * static_cast<float>(i),
              y, color);
    }
}

} // namespace

void Economy::reset() {
    coins_ = 3 * MELEE_COST;
    earnedFromDrops_ = 0;
    drops_.clear();
}

// One defeated enemy drops 10 coins.
void Economy::addDrop(int lane, float enemyX) {
    drops_.push_back({lane, enemyX, MELEE_COST});
}

// Pick up coins when a living player touches them.
void Economy::update(const PlayerManager& players) {
    const auto live = players.snapshots();

    drops_.erase(
        std::remove_if(
            drops_.begin(),
            drops_.end(),
            [&](const Drop& drop) {
                for (const auto& p : live) {
                    if (p.lane == drop.lane &&
                        std::abs(p.x - drop.x) <=
                            p.halfWidth + 19.0f) {

                        coins_ += drop.value;
                        earnedFromDrops_ += drop.value;
                        return true;
                    }
                }

                return false;
            }
        ),
        drops_.end()
    );
}

// Determine if the player card is unlocked and affordable.
bool Economy::available(PlayerKind kind) const {
    const int price = cost(kind);

    // Initially only melee is unlocked.
    const bool discovered =
        kind == PlayerKind::Melee ||
        earnedFromDrops_ >= price;

    return discovered && coins_ >= price;
}

// Click a card after selecting a lane.
void Economy::handleCardClick(
    const Game& game,
    PlayerManager& players
) {
    if (!game.input.leftClicked()) return;

    auto lanes = game.lanes.selectedLanes();
    if (lanes.empty()) return;

    for (int i = 0; i < 3; ++i) {
        if (!inside(
                game.input.mouse,
                cardX(i), CARD_Y, CARD_W, CARD_H
            )) {
            continue;
        }

        PlayerKind kind = typeAt(i);
        if (!available(kind)) return;

        std::optional<PlayerManager::PlayerId> placed;

        if (kind == PlayerKind::Melee) {
            placed = players.placeMelee(lanes.front());
        }
        else if (kind == PlayerKind::LongRange) {
            placed = players.placeLongRange(lanes.front());
        }
        else {
            placed = players.placeDefense(lanes.front());
        }

        // Only subtract money after successful placement.
        if (placed)
            coins_ -= cost(kind);

        return;
    }
}

// Draw an Ankh-shaped coin on the existing ground.
void Economy::drawCollectibles(Renderer& r) const {
    for (const Drop& d : drops_) {
        float y = laneBottomY(d.lane) + 20.0f;

        r.circle(
            {d.x, y}, 18.0f,
            {0.75f, 0.45f, 0.08f, 1.0f}
        );

        r.circle(
            {d.x, y}, 14.0f,
            {1.0f, 0.83f, 0.27f, 1.0f}
        );

        // Small Ankh on the coin.
        glm::vec4 symbol{0.52f, 0.29f, 0.06f, 1.0f};

        r.ellipse({d.x, y+5.5f}, {4, 5.5f}, symbol);
        r.rect({d.x, y-5}, {3, 13}, symbol);
        r.rect({d.x, y-1}, {11, 3}, symbol);
    }
}

// Draw cards and the coin counter in the top HUD strip.
void Economy::drawHud(
    Renderer& r,
    glm::vec2 mouse
) const {
    for (int i = 0; i < 3; ++i) {
        PlayerKind kind = typeAt(i);

        bool open = available(kind);
        float x = cardX(i);
        float y = CARD_Y;

        bool hover = open &&
            inside(mouse, x, y, CARD_W, CARD_H);

        glm::vec4 bg = open
            ? glm::vec4(0.19f, 0.28f, 0.32f, 1.0f)
            : glm::vec4(0.30f, 0.30f, 0.30f, 1.0f);

        glm::vec4 ink = open
            ? glm::vec4(0.96f, 0.77f, 0.26f, 1.0f)
            : glm::vec4(0.52f, 0.52f, 0.52f, 1.0f);

        r.rectBL(
            {x, y}, {CARD_W, CARD_H},
            hover
                ? glm::vec4(0.31f, 0.39f, 0.38f, 1.0f)
                : bg
        );

        // Melee card: sword.
        if (kind == PlayerKind::Melee) {
            r.push();
            r.translate({x+66, y+66});
            r.rotate(-36.0f);

            r.rect({0,0}, {9,52}, ink);
            r.rect({0,-16}, {30,6}, ink);

            r.pop();
        }

        // Long-range card: bow and arrow.
        else if (kind == PlayerKind::LongRange) {
            r.ellipse({x+65,y+68}, {15,33}, ink);
            r.ellipse({x+69,y+68}, {13,28}, bg);
            r.rect({x+66,y+68}, {2,57}, ink);
            r.triangle({x+93,y+69}, {13,11}, ink);
        }

        // Defense card: shield.
        else {
            r.polygon({
                {x+48,y+92}, {x+83,y+92},
                {x+87,y+58}, {x+65,y+43},
                {x+44,y+58}
            }, ink);
        }

        // Card price.
        number(r, cost(kind), x+56, y+8, ink);
    }

    // Wallet in the top-right corner.
    float x = 1305.0f;
    float y = CARD_Y;

    r.rectBL(
        {x,y}, {255,111},
        {0.13f,0.20f,0.24f,1.0f}
    );

    r.circle(
        {x+52,y+58}, 31,
        {0.97f,0.71f,0.13f,1.0f}
    );

    r.circle(
        {x+52,y+58}, 23,
        {1.0f,0.88f,0.36f,1.0f}
    );

    number(
        r, coins_, x+102, y+41,
        {1.0f,0.91f,0.55f,1.0f}
    );
}
