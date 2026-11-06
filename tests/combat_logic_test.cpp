
// Automated tests for the integrated players, enemies and economy.
// No game window is opened.
// Make the test-only enemy interface visible to IntelliSense.
#ifndef TEMPLE_COMBAT_TESTS
#define TEMPLE_COMBAT_TESTS
#endif

#include "PlayerManager.h"
#include "enemy/EnemySystem.h"
#include "game/Economy.h"
#include "ui/Cards.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int checks = 0;

void check(bool good, const char* description) {
    ++checks;

    if (!good) {
        throw std::runtime_error(description);
    }
}

// Return a living player's current health.
int healthOf(const PlayerManager& p, std::size_t id) {
    for (const auto& x : p.snapshots()) {
        if (x.id == id) return x.health;
    }

    return 0;
}

float positionOf(const PlayerManager& p, std::size_t id) {
    for (const auto& x : p.snapshots()) {
        if (x.id == id) return x.x;
    }

    return -1.0f;
}

// Simulate gameplay without rendering.
void tick(
    Game& game,
    PlayerManager& players,
    EnemySystem& enemies,
    float seconds,
    int jumpOnFrame = -1,
    bool duck = false
) {
    constexpr float dt = 1.0f / 60.0f;
    const int frames = static_cast<int>(seconds / dt);

    for (int i = 0; i < frames && game.isPlaying(); ++i) {
        players.setEngagements(enemies.enemies(), dt);

        players.update(
            dt,
            {0, 1, 2, 3, 4},
            i == jumpOnFrame,
            duck
        );

        enemies.update(game, players, dt);
    }
}

// TA RULES + COIN SYSTEM
void testCardsAndPlacement() {
    Game game;
    PlayerManager players;
    Economy wallet;
    CardsUI cards;

    cards.setup(game, players, wallet);

    check(wallet.coins == 30,
          "Game starts with 30 coins");

    check(cards.isUnlocked(0),
          "Melee starts unlocked");

    check(!cards.isUnlocked(1) && !cards.isUnlocked(2),
          "Advanced players start locked");

    // Money alone must not bypass collectible unlocking.
    wallet.add(100);

    check(!cards.isUnlocked(1) && !cards.isUnlocked(2),
          "Advanced unlock requires enemy pickups");

    wallet.reset();

    // No selected lane.
    players.selectType(PlayerKind::Melee);

    check(!players.placeOnSelectedLane(game),
          "Cannot place with no selected lane");

    // Select two lanes.
    game.lanes.update({300, laneCenterY(0)}, true);
    game.lanes.update({300, laneCenterY(1)}, true);

    check(!players.placeOnSelectedLane(game),
          "Cannot place with two selected lanes");

    // Deselect the second lane.
    game.lanes.update({300, laneCenterY(1)}, true);

    auto attacker = players.placeOnSelectedLane(game);

    check(attacker.has_value(),
          "One selected lane permits placement");

    check(players.snapshots().size() == 1 &&
          players.snapshots()[0].lane == 0,
          "Player appears in the selected lane");

    // TA shield restriction.
    check(!players.placeDefense(2).has_value(),
          "Shield rejected in a lane without an attacker");

    check(players.placeDefense(0).has_value(),
          "Shield allowed when a living attacker exists");

    // Defeat the shield, followed by its attacker.
    players.applyEnemyAttack(
        0, 900, FIELD_X0 - 50.0f,
        10, AttackHeight::Melee
    );

    players.applyEnemyAttack(
        0, 900, FIELD_X0 - 50.0f,
        10, AttackHeight::Melee
    );

    check(!players.placeDefense(0),
          "A dead attacker cannot authorize a new shield");

    // Test collectible-based unlocking.
    players.clear();
    players.placeMelee(0, 1000);

    wallet.addDrop(0, 1000);
    wallet.update(players);

    check(wallet.coins == 40 &&
          wallet.earnedFromDrops() == 10,
          "First enemy collectible picked up");

    check(!cards.isUnlocked(2),
          "Long Range remains locked after one drop");

    wallet.addDrop(0, 1000);
    wallet.update(players);

    check(cards.isUnlocked(2),
          "Long Range unlocks after 20 earned coins");

    wallet.addDrop(0, 1000);
    wallet.update(players);

    check(cards.isUnlocked(1),
          "Defense unlocks after 30 earned coins");

    check(wallet.spend(50),
          "Original Defense purchase price is preserved");

    check(!cards.isUnlocked(1),
          "Defense locks when remaining wallet is insufficient");

    wallet.reset();

    check(wallet.coins == 30 &&
          wallet.earnedFromDrops() == 0,
          "Restart resets money and unlock progress");
}

// ENEMY DIFFICULTIES + WAVES
void testEnemyRolesAndWaves() {
    Game game;
    PlayerManager players;
    EnemySystem enemies;

    enemies.spawnForTest(EnemyKind::Medusa, 0, 1450);
    enemies.spawnForTest(EnemyKind::Mummy, 1, 1450);
    enemies.spawnForTest(EnemyKind::Pharaoh, 2, 1450);

    const auto& all = enemies.enemies();

    check(all.size() == 3,
          "All three enemy types spawn");

    check(all[0].maxHealth == 5 &&
          all[0].attackDamage == 1,
          "Medusa: Easy, 5 HP, 1 damage");

    check(all[1].maxHealth == 5 &&
          all[1].attackDamage == 1,
          "Mummy: Medium, 5 HP, 1 damage");

    check(all[2].maxHealth == 7 &&
          all[2].attackDamage == 2,
          "Pharaoh: Hard, 7 HP, 2 damage");

    check(all[2].speed > all[1].speed &&
          all[0].speed == all[1].speed,
          "Pharaoh moves fastest");

    EnemySystem waves;

    tick(game, players, waves, 5.5f);

    check(waves.wave() == 1 &&
          waves.enemies().size() == 3,
          "Wave 1 spawns three enemies");

    for (const auto& e : waves.enemies()) {
        check(e.kind == EnemyKind::Medusa,
              "Wave 1 has only Easy enemies");
    }

    // Defeat all enemies from Wave 1.
    for (std::size_t i = 0; i < waves.enemies().size(); ++i) {
        waves.damageEnemy(i, 99);
    }

    tick(game, players, waves, 0.1f);

    check(waves.wave() == 2,
          "Wave 2 starts after Wave 1 clears");

    tick(game, players, waves, 11.2f);

    check(waves.enemies().size() == 5,
          "Wave 2 spawns five enemies");

    bool sawEasy = false;
    bool sawMedium = false;

    for (const auto& e : waves.enemies()) {
        sawEasy |= e.kind == EnemyKind::Medusa;
        sawMedium |= e.kind == EnemyKind::Mummy;

        check(e.kind != EnemyKind::Pharaoh,
              "Wave 2 has no Hard enemies");
    }

    check(sawEasy && sawMedium,
          "Wave 2 mixes Easy and Medium");
}

// REAL ATTACK DAMAGE
void testActualCombat() {
    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        auto id = *players.placeLongRange(0, 1100);

        enemies.spawnForTest(EnemyKind::Medusa, 0, 1400);

        tick(game, players, enemies, 0.55f);

        check(healthOf(players, id) == 4,
              "Medusa beam deals one damage");
    }

    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        auto id = *players.placeLongRange(0, 1100);

        enemies.spawnForTest(EnemyKind::Medusa, 0, 1400);

        tick(game, players, enemies, 0.55f, -1, true);

        check(healthOf(players, id) == 5,
              "Ducking avoids Medusa beam");
    }

    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        players.placeMelee(1, 1100);
        auto shield = *players.placeDefense(1, 1200);

        enemies.spawnForTest(EnemyKind::Mummy, 1, 1370);

        tick(game, players, enemies, 1.4f);

        check(healthOf(players, shield) == 6,
              "Mummy tornado damages the shield once");
    }

    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        auto id = *players.placeMelee(1, 1200);

        enemies.spawnForTest(EnemyKind::Mummy, 1, 1370);

        tick(game, players, enemies, 1.4f, 40);

        check(healthOf(players, id) == 5,
              "Jumping avoids the low tornado");
    }

    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        auto id = *players.placeMelee(2, 1200);

        enemies.spawnForTest(EnemyKind::Pharaoh, 2, 1260);

        tick(game, players, enemies, 0.78f);

        check(healthOf(players, id) == 3,
              "Pharaoh deals two damage at close range");

        check(enemies.enemies()[0].x > positionOf(players, id),
              "Pharaoh stops before the player");
    }

    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        auto id = *players.placeMelee(2, 1100);

        enemies.spawnForTest(EnemyKind::Pharaoh, 2, 1490);

        tick(game, players, enemies, 0.78f);

        check(healthOf(players, id) == 5,
              "Pharaoh cannot strike from a distance");
    }

    {
        PlayerManager players;
        EnemySystem enemies;

        auto id = *players.placeMelee(2, 1200);

        enemies.spawnForTest(EnemyKind::Pharaoh, 2, 1240);

        players.setEngagements(enemies.enemies(), 0.1f);
        players.update(0.1f);

        check(std::abs(positionOf(players, id) - 1200) < 0.01f,
              "Player stops when touching an enemy");

        enemies.damageEnemy(0, 7);

        players.setEngagements(enemies.enemies(), 0.1f);
        players.update(0.1f);

        check(positionOf(players, id) > 1200,
              "Player resumes moving after enemy death");
    }
}

// ARROW TARGETING + MULTIPLE LANE DODGING
void testArrowOrderAndMultiLaneDodge() {
    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        players.placeLongRange(3, 1100);

        enemies.spawnForTest(EnemyKind::Mummy, 3, 1270);
        enemies.spawnForTest(EnemyKind::Medusa, 3, 1450);

        tick(game, players, enemies, 1.25f);

        int nearHP = 5;
        int farHP = 5;

        for (const auto& e : enemies.enemies()) {
            if (e.kind == EnemyKind::Mummy) nearHP = e.health;
            if (e.kind == EnemyKind::Medusa) farHP = e.health;
        }

        check(nearHP < 5,
              "Archer hits nearer enemy first");

        check(farHP == 5,
              "Arrow does not pass through the nearer enemy");
    }

    {
        PlayerManager players;

        auto a = *players.placeLongRange(0, 1000);
        auto b = *players.placeLongRange(1, 1000);

        // Both lanes receive the same duck command.
        players.update(0.01f, {0, 1}, false, true);

        check(
            !players.applyEnemyAttack(
                0, 1200, 900, 1, AttackHeight::High
            ),
            "Selected lane 0 ducks"
        );

        check(
            !players.applyEnemyAttack(
                1, 1200, 900, 1, AttackHeight::High
            ),
            "Selected lane 1 ducks"
        );

        check(healthOf(players, a) == 5 &&
              healthOf(players, b) == 5,
              "Both selected lanes avoid damage");
    }
}

// ENEMY DROPS + BASE LOSS
void testDropsAndBase() {
    {
        EnemySystem enemies;
        Economy wallet;

        enemies.onEnemyDefeated([&](int lane, float x) {
            wallet.addDrop(lane, x);
        });

        enemies.spawnForTest(EnemyKind::Mummy, 0, 1000);

        enemies.damageEnemy(0, 5);
        enemies.damageEnemy(0, 5);

        PlayerManager players;
        players.placeMelee(0, 1000);

        wallet.update(players);

        check(wallet.earnedFromDrops() == 10,
              "One defeated enemy drops only one collectible");
    }

    {
        Game game;
        PlayerManager players;
        EnemySystem enemies;

        enemies.spawnForTest(
            EnemyKind::Medusa, 4, FIELD_X0 + 1
        );

        tick(game, players, enemies, 0.1f);

        check(game.state() == GameState::GameOver &&
              !game.playerWon(),
              "Base breach ends the game");
    }
}

} // namespace

int main() {
    try {
        testCardsAndPlacement();
        testEnemyRolesAndWaves();
        testActualCombat();
        testArrowOrderAndMultiLaneDodge();
        testDropsAndBase();

        std::cout
            << "PASS: " << checks
            << " integrated checks\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
