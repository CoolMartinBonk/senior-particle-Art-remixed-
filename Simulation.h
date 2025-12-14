#pragma once
#include "GameConfig.h"
#include "SpatialGrid.h"
#include "ThreadPool.h"

void update_physics_simulation(bool brushMode, int mx, int my, bool mouseDown, bool playerSunMode, bool playerRainbow, float& centerX, float& centerY, float& avgVx, float& avgVy, float& playerRainbowTimer, float& playerJumpTimer, SpatialGrid & grid, ThreadPool & pool);

void update_meteors(float& timer, float& interval, bool silent, int& TARGET_FPS);