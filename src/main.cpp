#include "core/Game.h"
#include "enemy/EnemySystem.h"
#include "PlayerManager.h"
#include "game/Economy.h"
#include "ui/Cards.h"


int main() {
    // Original Game initialization
    Game game;
    PlayerManager players;
    players.setup(game);

    // ========== CARDS + COIN COUNTER (D) ==========
    // Cards place players through players.selectType()/placeOnSelectedLane() and spend coins.
    // C: add coins with economy.add(value) when a collectible is picked up.
    Economy economy;
    CardsUI cards;
    cards.setup(game, players, economy);

    // ---- TEMPORARY coin test keys (delete when C's collectibles exist) ----
    // P = +10 coins, O = -10 coins. Lets you watch cards unlock / lock again.
    game.onGameplayUpdate([&](float) {
        if (game.input.keyJustPressed(GLFW_KEY_P)) economy.add(10);
        if (game.input.keyJustPressed(GLFW_KEY_O)) economy.coins = economy.coins >= 10 ? economy.coins - 10 : 0;
    });
    if (!game.init()) return -1;

    // ========== ENEMIES ==========

    EnemySystem enemies;

    // Create the mummy, Pharaoh and Medusa
    enemies.reset();

    // Move enemies and update their attacks every frame
    game.onGameplayUpdate([&](float dt) {
        enemies.update(game, dt);

        // ---- BRIDGE (B): players -> enemies --------------------------------
        // Nothing else calls PlayerManager::applyPlayerAttacksToEnemy(), so
        // without this the players animate and shoot but never hurt anyone.
        // Runs once per enemy per frame; each landed hit is 1 damage.
        // (Players update before enemies because players.setup() registered first.)
        constexpr float ENEMY_HALF_W = 30.0f;   // enemy body half-width in virtual px
        for (std::size_t i = 0; i < enemies.enemies().size(); ++i) {
            const Enemy e = enemies.enemies()[i];   // copy: damageEnemy() mutates the list
            players.notifyEnemyContact(e.lane, e.x - ENEMY_HALF_W, e.x + ENEMY_HALF_W);
            const int dmg = players.applyPlayerAttacksToEnemy(
                e.lane, e.x - ENEMY_HALF_W, e.x + ENEMY_HALF_W);
            if (dmg > 0) enemies.damageEnemy(i, dmg);
        }
    });

    // Draw enemies on top of the existing lanes and ground
    game.onDraw(Layer::Entities, [&](Renderer& r) {
        enemies.drawEnemies(r);
    });

    // Draw projectiles, lasers and health bars
    game.onDraw(Layer::Effects, [&](Renderer& r) {
        enemies.drawAttacksAndHealth(r);
    });

    // Reset enemies when restarting the game
    game.onReset([&]() {
        enemies.reset();
    });

    // ========== OTHER TEAMMATES ==========
    // Your colleague can add player code here.

    // Original Game loop
    game.run();
    game.shutdown();

    return 0;
}