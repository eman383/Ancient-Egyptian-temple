
#include "core/Game.h"
#include "enemy/EnemySystem.h"
#include "PlayerManager.h"
#include "game/Economy.h"

int main() {
    Game game;
    PlayerManager players;
    EnemySystem enemies;
    Economy economy;

    // CHANGED: register drawing/reset hooks, but do NOT register an independent
    // player update. Our single callback below controls the combat update order.
    players.setup(game, false);

    if (!game.init()) return -1;

    // PRESERVED: one collectible drops whenever an enemy is defeated.
    enemies.onEnemyDefeated([&](int lane, float x) {
        economy.addDrop(lane, x);
    });
    enemies.reset();
    economy.reset();

    game.onGameplayUpdate([&](float dt) {
        // PRESERVED: player-card purchasing and initial economy.
        economy.handleCardClick(game, players);

        // CHANGED: contact must be found BEFORE player movement.
        players.setEngagements(enemies.enemies(), dt);
        players.update(dt, game.lanes.selectedLanes(), game.input.jumpPressed(),
                       game.input.key(GLFW_KEY_S) || game.input.key(GLFW_KEY_DOWN));

        // CHANGED: this updates enemies, resolves both sides' attacks, and runs waves.
        enemies.update(game, players, dt);

        // PRESERVED: players automatically collect dropped coins on contact.
        if (game.isPlaying()) economy.update(players);
    });

    game.onDraw(Layer::Entities, [&](Renderer& r) {
        enemies.drawEnemies(r);
        economy.drawCollectibles(r);
    });
    game.onDraw(Layer::Effects, [&](Renderer& r) {
        enemies.drawAttacksAndHealth(r);
    });
    game.onDraw(Layer::Hud, [&](Renderer& r) {
        economy.drawHud(r, game.input.mouse);
    });

    // Temporary restart key until the final game menus are built.
    game.onAlwaysUpdate([&](float) {
        if (game.input.keyJustPressed(GLFW_KEY_R)) game.restart();
    });
    game.onReset([&] {
        enemies.reset();
        economy.reset();
    });

    game.run();
    game.shutdown();
    return 0;
}
