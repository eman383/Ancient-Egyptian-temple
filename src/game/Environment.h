#pragma once
#include "render/Renderer.h"

// Ancient-Egyptian scenery. Three layers, drawn in this order by Game::drawScene():
//
//   drawBackdrop(r, time)    -> BEFORE the lanes. Sky, sun, dunes, pyramids, palms, obelisks
//                               in the top strip (y 750..900). D draws the UI cards on top.
//   lanes.drawLanes(r)
//   drawFieldDecor(r, time)  -> AFTER the lanes, BEFORE characters. Jars and brazier stands
//                               in the lanes plus the tomb-door wall on the right edge
//                               (enemies spawn at x = FIELD_X1, right at those doors).
//
// Decor is small and sits near lane edges so players and enemies stay readable.
void drawBackdrop(Renderer& r, float time);
void drawFieldDecor(Renderer& r, float time);