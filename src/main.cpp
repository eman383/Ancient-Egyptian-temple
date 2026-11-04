
#include "core/Game.h"
#include "Enemy/EnemySystem.h"

int main() {
    // Original Game initialization
    Game game;
    if (!game.init()) return -1;

    // ========== ENEMIES ==========

    EnemySystem enemies;

    // Create the mummy, Pharaoh and Medusa
    enemies.reset();

    // Move enemies and update their attacks every frame
    game.onGameplayUpdate([&](float dt) {
        enemies.update(game, dt);
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
