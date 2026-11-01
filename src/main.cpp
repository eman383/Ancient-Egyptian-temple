#include "core/Game.h"
#include "PlayerManager.h"

int main() {
    Game game;
    PlayerManager players;
    players.setup(game);
    players.placeMelee(2);
    players.placeDefense(3);
    players.placeLongRange(4);
    if (!game.init()) return -1;
    game.run();
    game.shutdown();
    return 0;
}
