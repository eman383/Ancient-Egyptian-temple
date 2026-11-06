
#include "core/Game.h"
#include "enemy/EnemySystem.h"
#include "PlayerManager.h"
#include "game/Economy.h"
#include "ui/Cards.h"
#include "ui/TempleMenus.h"

int main() {
    Game game;
    PlayerManager players;
    EnemySystem enemies;
    Economy economy;
    CardsUI cards;
    TempleMenus menus;

    // COMBINED:
    // Keep your teammate's player drawing and reset hooks.
    // The 'false' prevents a separate player movement update.
    players.setup(game, false);

    // PLAYER_FINAL:
    // Keep her player cards, grayscale locking, model previews
    // and coin counter.
    // Card purchasing is registered before combat movement.
    cards.setup(game, players, economy);

    // Animated 15-second intro, Start button, instructions, pause, win/lose menus.
    menus.setup(game);

    // ENEMIES:
    // Each defeated enemy creates one collectible.
    enemies.onEnemyDefeated([&](int lane, float x) {
        economy.addDrop(lane, x);
    });

    enemies.reset();

    // COMBINED GAMEPLAY UPDATE:
    // Keep the order consistent every frame.
    game.onGameplayUpdate([&](float dt) {

        // 1. Find enemies about to collide with players.
        players.setEngagements(enemies.enemies(), dt);

        // 2. Move players and handle jump/duck for selected lanes.
        players.update(
            dt,
            game.lanes.selectedLanes(),
            game.input.jumpPressed(),
            game.input.key(GLFW_KEY_S) ||
                game.input.key(GLFW_KEY_DOWN)
        );

        // 3. Move enemies, apply attacks and handle waves.
        enemies.update(game, players, dt);

        // 4. Collect coins touched by surviving players.
        if (game.isPlaying()) {
            economy.update(players);
        }
    });

    // Render enemies and collectible coins.
    // PlayerManager and CardsUI already register their
    // own drawing callbacks.
    game.onDraw(Layer::Entities, [&](Renderer& r) {
        enemies.drawEnemies(r);
        economy.drawCollectibles(r);
    });

    // Enemy lasers, tornadoes, lightning and health bars.
    game.onDraw(Layer::Effects, [&](Renderer& r) {
        enemies.drawAttacksAndHealth(r);
    });

    // Reset waves, enemies, coins, drops and unlock progress.
    game.onReset([&] {
        enemies.reset();
        economy.reset();
    });

    if (!game.init()) return -1;

    game.run();
    game.shutdown();

    return 0;
}
