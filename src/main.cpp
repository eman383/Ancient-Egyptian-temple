
#include "core/Game.h"
#include "enemy/EnemySystem.h"
#include "PlayerManager.h"
#include "game/Economy.h"
#include "ui/Cards.h"
#include "ui/TempleMenus.h"
#include "audio/Audio.h"

#include <algorithm>

int main() {
    Game game;
    PlayerManager players;
    EnemySystem enemies;
    Economy economy;
    CardsUI cards;
    TempleMenus menus;

    // Player drawing and reset hooks. 'false': movement is updated in the
    // single ordered gameplay update below instead.
    players.setup(game, false);

    // Player cards (grayscale when locked), model previews and coin counter.
    // Card purchasing is registered before combat movement.
    cards.setup(game, players, economy);

    // A shield can be dropped anywhere in its lane, but never behind or inside an enemy.
    cards.setPlacementLimit([&](int lane) {
        float limit = FIELD_X1;
        for (const Enemy& e : enemies.enemies())
            if (e.lane == lane && e.health > 0)
                limit = std::min(limit, e.x - e.halfWidth() - 0.23f * LANE_H * 0.80f - 4.0f);
        return limit;
    });

    // Main menu + sound settings, 17-second intro cutscene, instructions, pause, win/lose menus.
    menus.setup(game);

    // Each defeated enemy drops one collectible.
    enemies.onEnemyDefeated([&](int lane, float x) {
        economy.addDrop(lane, x);
    });

    enemies.reset();

    // Gameplay update:
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
    audio::init();          // music + effects; the game still runs if there is no audio device

    game.run();
    audio::shutdown();
    game.shutdown();

    return 0;
}
