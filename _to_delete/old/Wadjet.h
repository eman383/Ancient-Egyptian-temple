#pragma once

#include "EnemyTypes.h"
#include "render/Renderer.h"

// Draw Wadjet
void drawWadjet(Renderer& r, const Enemy& enemy);

// Draw the red lasers coming from her eyes
void drawWadjetLaser(Renderer& r, const Enemy& enemy);
