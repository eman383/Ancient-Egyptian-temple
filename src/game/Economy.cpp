
#include "Economy.h"

#include "PlayerManager.h"
#include "Config.h"
#include "render/Renderer.h"
#include "ui/Cards.h"

#include <algorithm>
#include <cmath>

// Reset the economy when restarting the game.
void Economy::reset() {
    coins = STARTING_COINS;
    earnedFromDrops_ = 0;
    drops_.clear();
}

// Add money without counting it as an enemy collectible.
void Economy::add(int amount) {
    if (amount > 0) {
        coins += amount;
    }
}

// CardsUI uses this function to determine whether a
// player card is available for purchasing.
bool Economy::canAfford(int cost) const {

    // The wallet must contain enough money.
    if (cost < 0 || coins < cost) {
        return false;
    }

    // Preserve the original prices in Cards.h.
    const int melee = cards::cost(PlayerKind::Melee);

    // Long Range unlocks after 2x Melee collectibles.
    if (cost == cards::cost(PlayerKind::LongRange) &&
        earnedFromDrops_ < 2 * melee) {
        return false;
    }

    // Defense unlocks after 3x Melee collectibles.
    if (cost == cards::cost(PlayerKind::Defense) &&
        earnedFromDrops_ < 3 * melee) {
        return false;
    }

    return true;
}

// Deduct money after a successful player placement.
bool Economy::spend(int cost) {
    if (!canAfford(cost)) {
        return false;
    }

    coins -= cost;
    return true;
}

// ENEMY INTEGRATION:
// Add one collectible at the defeated enemy's position.
void Economy::addDrop(int lane, float x) {
    if (lane < 0 || lane >= LANE_COUNT) {
        return;
    }

    drops_.push_back({
        lane,
        x,
        10
    });
}

// Check whether any living player touches a collectible.
void Economy::update(const PlayerManager& players) {

    const auto living = players.snapshots();

    drops_.erase(
        std::remove_if(
            drops_.begin(),
            drops_.end(),
            [&](const Drop& drop) {

                for (const auto& player : living) {

                    // Same lane and close enough horizontally.
                    if (player.lane == drop.lane &&
                        std::abs(player.x - drop.x) <=
                            player.halfWidth + 19.0f) {

                        // Add the coin value to the wallet.
                        coins += drop.value;

                        // Also increase unlock progress.
                        earnedFromDrops_ += drop.value;

                        // Remove this collected coin.
                        return true;
                    }
                }

                return false;
            }
        ),
        drops_.end()
    );
}

// Draw the dropped coins using the existing Renderer.
void Economy::drawCollectibles(Renderer& r) const {

    for (const Drop& d : drops_) {

        const float y = laneBottomY(d.lane) + 20.0f;

        // Dark outer coin ring.
        r.circle(
            {d.x, y},
            18.0f,
            {0.75f, 0.45f, 0.08f, 1.0f}
        );

        // Golden center.
        r.circle(
            {d.x, y},
            14.0f,
            {1.0f, 0.83f, 0.27f, 1.0f}
        );

        // Egyptian Ankh symbol.
        const glm::vec4 symbol{
            0.52f, 0.29f, 0.06f, 1.0f
        };

        r.ellipse(
            {d.x, y + 5.5f},
            {4.0f, 5.5f},
            symbol
        );

        r.rect(
            {d.x, y - 5.0f},
            {3.0f, 13.0f},
            symbol
        );

        r.rect(
            {d.x, y - 1.0f},
            {11.0f, 3.0f},
            symbol
        );
    }
}
