# Project conventions (Part A: engine) — read before writing code

Theme: **Ancient Egyptian temple**. Players defend the temple pylon on the left; mummies and
other undead come out of the tomb doors on the right.

## Build
- Open the project folder in VS Code, **CMake: Configure**, Build, Launch.
- Every `.cpp` under `src/` is compiled automatically. **After adding a new .cpp, run
  CMake: Configure again.**
- OpenGL 3.3 core. GLAD 2 lives only in `src/glad_impl.cpp` — never define
  `GLAD_GL_IMPLEMENTATION` anywhere else.
- Put your code in your own files/folders to avoid merge conflicts:
  B -> `src/game/players/`, C -> `src/game/enemies/`, D -> `src/ui/`. Don't edit `Game.cpp`
  unless it is a real engine change; use the hooks below.

## Coordinates (see `src/Config.h`)
- Virtual screen **1600 x 900**, origin **bottom-left**, +x right, +y up. Always use these
  units; the engine scales them to the window.
- UI/scenery strip: y 750..900. Lane area: y 0..750. **5 lanes, 150 high each.**
- Lane 0 = TOP lane, lane 4 = BOTTOM lane.
- `laneCenterY(i)`, `laneBottomY(i)`, `laneFromY(y)` — use these, never hard-code numbers.
- Home base: x 0..200. **Players spawn at x = FIELD_X0 (200).**
  **Enemies spawn at x = FIELD_X1 (1600)**, right at the tomb doors.
- **Lose condition: an enemy with x <= FIELD_X0.** Then call `game.lose()`.
- 10 visual columns of width `CELL_W` (140) — placement is per lane, columns are only for looks.

## Drawing models (`src/render/Renderer.h`)
Build every character from several shapes with the transform stack. Models are NOT allowed to
be a single primitive. Model rules:
- Origin = **feet-centre**. Models face **right**; enemies are drawn mirrored with
  `r.scale({-1, 1})`.
- Different roles need different SHAPES and equipment, not just a different colour.
```cpp
r.push();
  r.translate({x, y});                      // feet-centre
  r.rect({0, 45}, {44, 70}, bodyColor);     // body
  r.circle({0, 95}, 18, skinColor);         // head
  r.push();
    r.translate({24, 70});                  // shoulder
    r.rotate(armAngle);                     // animate: children follow the parent
    r.rect({0, -22}, {10, 44}, armColor);
  r.pop();
r.pop();
```
Shapes: `rect, rectBL, triangle, circle, ellipse, polygon` (convex or simple fan shapes).
Damage tint: draw the body colour mixed toward red as health drops, or scale the model down.
Health must always be visible (bar or number) above/below every player and enemy.

## Lanes (`game.lanes`)
- `isSelected(i)`, `selectedLanes()`, `hovered()`, `anySelected()`
- Click toggles a lane; several lanes can be selected. B: **W/Up = jump, S/Down = duck** for every
  character in selected lanes (`game.input.jumpPressed()` / `duckPressed()`; there is a temporary
  console print in `Game::update` to delete).

## Game flow and hooks (`src/core/Game.h`)
States: `MainMenu -> Cutscene -> Playing <-> Paused`, `Playing -> GameOver (won or lost)`.

Commands (call from anywhere):
`startGame()` (menu Start), `finishCutscene()`, `pause()`, `resume()`, `restart()`,
`goToMainMenu()`, `win()`, `lose()`, `requestExit()`.
Esc already pauses/resumes. `win()`/`lose()` freeze everything (gameplay hooks stop running).

Hooks — register ONCE at startup (e.g. in a `setupX(Game& game)` function called from `main.cpp`):
```cpp
game.onGameplayUpdate([&](float dt){ /* move, attack, spawn waves */ });  // only while Playing
game.onAlwaysUpdate  ([&](float dt){ /* menus, cutscene timing */ });     // every state
game.onDraw(Layer::Entities, [&](Renderer& r){ /* players, enemies, collectibles */ });
game.onDraw(Layer::Effects,  [&](Renderer& r){ /* projectiles, health bars */ });
game.onDraw(Layer::Hud,      [&](Renderer& r){ /* cards, coin counter */ });
game.onDraw(Layer::Menus,    [&](Renderer& r){ /* menus + cutscene; check game.state() */ });
game.onReset         ([&]{ /* clear your lists, coins back to start */ });  // restart / to menu
```
Draw order: backdrop -> lanes -> decor -> home base -> lane highlight -> Entities -> Effects ->
Hud -> pause/game-over tint -> Menus. Scene layers draw only in Playing/Paused/GameOver;
the Menus layer draws in every state.

## Who owns what
- **A:** window, loop, renderer, lanes, home base, environment, state machine, hooks.
- **B:** 3 player types, placement, attacks, jump/duck, damage and health.
- **C:** 3 enemy types, waves (3, 5, 10 enemies), coins/collectibles, lose/win calls.
- **D:** menus (main, pause, win/lose), player cards, coin counter, cutscene, video.

## Dev switches (top of `Game.cpp`)
- `START_STATE`: D sets it to `GameState::MainMenu` once the main menu exists.
- `DEV_KEYS`: 1 menu, 2 cutscene, 3 playing, 4 pause, 5 lose, 6 win, R restart. **Set to false
  before submitting.**

## Before submitting
- [ ] `DEV_KEYS = false`, `START_STATE = MainMenu`
- [ ] Remove the temporary JUMP/DUCK console print
- [ ] Clean build from scratch with no errors (compile errors = zero)
- [ ] Submit only source files in the required zip naming: `[Team name]-[Member name].zip`