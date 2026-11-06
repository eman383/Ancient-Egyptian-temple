#pragma once

#include "EnemyTypes.h"
#include "render/Renderer.h"

// Draw Medusa
void drawMedusa(Renderer& r, const Enemy& enemy);

// Draw the red lasers coming from her eyes
void drawMedusaLaser(Renderer& r, const Enemy& enemy);
