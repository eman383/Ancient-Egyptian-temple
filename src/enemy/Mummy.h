#pragma once

#include "EnemyTypes.h"
#include "render/Renderer.h"

// Draw the mummy character
void drawMummy(Renderer& r, const Enemy& enemy);

// Draw the mummy's wind attack
void drawMummyTornado(
    Renderer& r,
    const EnemyProjectile& wind
);
