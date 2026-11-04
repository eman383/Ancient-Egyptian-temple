#pragma once

#include "EnemyTypes.h"
#include "render/Renderer.h"

// Draw the Pharaoh
void drawPharaoh(Renderer& r, const Enemy& enemy);

// Draw his lightning projectile
void drawPharaohLightning(
    Renderer& r,
    const EnemyProjectile& bolt
);
