#pragma once
#include "render/Renderer.h"

// The player's home base: an Egyptian temple pylon on the left edge of the field.
// It covers x in [0, HOME_W] and all lanes, with one doorway per lane.
// Enemies LOSE the game for the player when they reach x <= FIELD_X0 (see Config.h).
//
// time is in seconds, only used for the torch flicker.
void drawHomeBase(Renderer& r, float time);