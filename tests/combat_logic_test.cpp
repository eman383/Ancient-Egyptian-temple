
 // NEW: checks the real combat classes without opening an OpenGL window.
 // Windows: cmake --build build-combat --target CombatLogicTests
 //          ctest --test-dir build-combat --output-on-failure
#include "PlayerManager.h"
#include "enemy/EnemySystem.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int checks = 0;
void check(bool ok, const std::string& description) {
    ++checks;
    if (!ok) throw std::runtime_error("FAILED: " + description);
}

int hp(const PlayerManager& players, std::size_t id) {
    for (const auto& p : players.snapshots()) if (p.id == id) return p.health;
    return 0; // Dead players are omitted from snapshots.
}

float xPosition(const PlayerManager& players, std::size_t id) {
    for (const auto& p : players.snapshots()) if (p.id == id) return p.x;
    return -1.0f;
}

void tick(Game& game, PlayerManager& players, EnemySystem& enemies,
          float seconds, int jumpAtFrame = -1, bool holdDuck = false) {
    constexpr float dt = 1.0f / 60.0f;
    const int frames = static_cast<int>(seconds / dt);
    for (int f = 0; f < frames && game.isPlaying(); ++f) {
        players.setEngagements(enemies.enemies(), dt);
        players.update(dt, {0, 1, 2, 3, 4}, f == jumpAtFrame, holdDuck);
        enemies.update(game, players, dt);
    }
}

void testRolesAndWaves() {
    Game game;
    PlayerManager players;
    EnemySystem enemies;
    enemies.reset();
    enemies.spawnForTest(EnemyKind::Medusa, 0, 1450);
    enemies.spawnForTest(EnemyKind::Mummy, 1, 1450);
    enemies.spawnForTest(EnemyKind::Pharaoh, 2, 1450);
    const auto& e = enemies.enemies();
    check(e.size() == 3, "all enemy classes can spawn");
    check(e[0].maxHealth == 5 && e[0].attackDamage == 1, "Medusa easy: 5 HP / 1 damage");
    check(e[1].maxHealth == 5 && e[1].attackDamage == 1, "Mummy medium: 5 HP / 1 damage");
    check(e[2].maxHealth == 7 && e[2].attackDamage == 2, "Pharaoh hard: 7 HP / 2 damage");
    check(e[2].speed > e[0].speed && e[0].speed == e[1].speed,
          "Pharaoh runs faster than both ranged enemies");

    EnemySystem waves;
    waves.reset();
    tick(game, players, waves, 5.5f);
    check(waves.wave() == 1 && waves.enemies().size() == 3, "wave 1 has three enemies");
    bool allEasy = true;
    for (const auto& enemy : waves.enemies()) allEasy &= enemy.kind == EnemyKind::Medusa;
    check(allEasy, "wave 1 contains only easy Medusas");
    for (std::size_t i = 0; i < waves.enemies().size(); ++i) waves.damageEnemy(i, 99);
    tick(game, players, waves, 0.1f);
    check(waves.wave() == 2, "wave 2 starts after wave 1 is cleared");
    tick(game, players, waves, 11.2f);
    check(waves.enemies().size() == 5, "wave 2 spawns five enemies");
    bool easy = false, medium = false, onlyAllowed = true;
    for (const auto& enemy : waves.enemies()) {
        easy |= enemy.kind == EnemyKind::Medusa;
        medium |= enemy.kind == EnemyKind::Mummy;
        onlyAllowed &= enemy.kind != EnemyKind::Pharaoh;
    }
    check(easy && medium && onlyAllowed, "wave 2 mixes easy and medium only");
}

void testDodgeAndDefense() {
    PlayerManager p;
    auto shield = *p.placeDefense(0, 1100);
    auto archer = *p.placeLongRange(0, 1000);
    check(p.applyEnemyAttack(0, 1400, 950, 1, AttackHeight::High).has_value(),
          "high laser hits first player in its path");
    check(hp(p, shield) == 6 && hp(p, archer) == 5, "shield protects archer behind");
    for (int i = 0; i < 6; ++i) p.applyEnemyAttack(0, 1400, 950, 1, AttackHeight::High);
    check(hp(p, shield) == 0, "defender dies after seven hits");

    p.update(0.01f, {0}, false, true);
    check(!p.applyEnemyAttack(0, 1400, 950, 1, AttackHeight::High),
          "duck avoids HIGH laser");
    check(hp(p, archer) == 5, "ducking preserves archer health");
    check(p.applyEnemyAttack(0, 1400, 950, 1, AttackHeight::Low).has_value(),
          "duck does not avoid LOW tornado");
    check(hp(p, archer) == 4, "tornado subtracts one hit point");

    PlayerManager jumper;
    auto melee = *jumper.placeMelee(2, 1200);
    jumper.update(0.01f, {2}, true, false);
    check(!jumper.applyEnemyAttack(2, 1350, 1100, 1, AttackHeight::Low),
          "jump avoids LOW tornado");
    check(jumper.applyEnemyAttack(2, 1350, 1100, 1, AttackHeight::High).has_value(),
          "jump cannot avoid HIGH laser");
    check(hp(jumper, melee) == 4, "jumping player loses one HP to high attack");
}

void testActualAttacks() {
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        auto archer = *p.placeLongRange(0, 1100);
        e.reset();
        e.spawnForTest(EnemyKind::Medusa, 0, 1400);
        tick(game, p, e, 0.55f);
        check(hp(p, archer) == 4, "real Medusa laser deals one damage at range");
    }
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        auto archer = *p.placeLongRange(0, 1100);
        e.reset();
        e.spawnForTest(EnemyKind::Medusa, 0, 1400);
        tick(game, p, e, 0.55f, -1, true);
        check(hp(p, archer) == 5, "duck avoids real Medusa laser");
    }
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        auto shield = *p.placeDefense(1, 1200);
        e.reset();
        e.spawnForTest(EnemyKind::Mummy, 1, 1370);
        tick(game, p, e, 1.4f);
        check(hp(p, shield) == 6, "real low tornado hits defender exactly once");
    }
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        auto melee = *p.placeMelee(1, 1200);
        e.reset();
        e.spawnForTest(EnemyKind::Mummy, 1, 1370);
        tick(game, p, e, 1.4f, 40);
        check(hp(p, melee) == 5, "real tornado misses a well-timed jump");
    }
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        auto guard = *p.placeMelee(2, 1100);
        e.reset();
        e.spawnForTest(EnemyKind::Pharaoh, 2, 1490);
        tick(game, p, e, 0.78f);
        check(hp(p, guard) == 5, "Pharaoh cannot hit from far away");
    }
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        auto guard = *p.placeMelee(2, 1200);
        e.reset();
        e.spawnForTest(EnemyKind::Pharaoh, 2, 1260);
        tick(game, p, e, 0.78f);
        check(hp(p, guard) == 3, "Pharaoh melee lightning deals two damage");
        check(e.enemies()[0].x > xPosition(p, guard), "Pharaoh stops before the player");
    }
}

void testContactAndResume() {
    PlayerManager players;
    EnemySystem enemies;
    enemies.reset();
    const auto guard = *players.placeMelee(2, 1200.0f);
    enemies.spawnForTest(EnemyKind::Pharaoh, 2, 1240.0f);
    players.setEngagements(enemies.enemies(), 0.1f);
    players.update(0.1f);
    check(std::abs(xPosition(players, guard) - 1200.0f) < 0.01f,
          "player stops when touching an enemy");
    enemies.damageEnemy(0, 7);
    players.setEngagements(enemies.enemies(), 0.1f);
    players.update(0.1f);
    check(xPosition(players, guard) > 1200.0f,
          "surviving player resumes after enemy dies");
}

void testDropsAndArrowTargeting() {
    {
        EnemySystem e;
        e.reset();
        int drops = 0;
        e.onEnemyDefeated([&](int, float) { ++drops; });
        e.spawnForTest(EnemyKind::Mummy, 1, 1400);
        e.damageEnemy(0, 5);
        e.damageEnemy(0, 5);
        check(drops == 1, "enemy death creates only one collectible drop");
    }
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        p.placeLongRange(3, 1100);
        e.reset();
        e.spawnForTest(EnemyKind::Mummy, 3, 1270);  // Nearer target
        e.spawnForTest(EnemyKind::Medusa, 3, 1450); // Farther target
        tick(game, p, e, 1.25f);
        int nearHp = 5, farHp = 5;
        for (const auto& enemy : e.enemies()) {
            if (enemy.kind == EnemyKind::Mummy) nearHp = enemy.health;
            if (enemy.kind == EnemyKind::Medusa) farHp = enemy.health;
        }
        check(nearHp < 5, "archer hits nearer enemy first");
        check(farHp == 5, "arrow does not jump through nearer enemy");
    }
    {
        Game game;
        PlayerManager p;
        EnemySystem e;
        e.reset();
        e.spawnForTest(EnemyKind::Medusa, 4, FIELD_X0 + 1.0f);
        tick(game, p, e, 0.1f);
        check(game.state() == GameState::GameOver && !game.playerWon(),
              "enemy reaching the base ends gameplay");
    }
}
} // namespace

int main() {
    try {
        testRolesAndWaves();
        testDodgeAndDefense();
        testActualAttacks();
        testContactAndResume();
        testDropsAndArrowTargeting();
        std::cout << "PASS: " << checks << " combat checks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
