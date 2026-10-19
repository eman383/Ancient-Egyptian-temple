#include "core/Game.h"

int main() {
    Game game;
    if (!game.init()) return -1;
    game.run();
    game.shutdown();
    return 0;
}