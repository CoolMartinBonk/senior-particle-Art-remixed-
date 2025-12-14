#pragma once
#include "GameConfig.h"

void HSVtoRGB(float h, float s, float v, Uint8& r, Uint8& g, Uint8& b);
void generateRainbowLUT();
void spawnExplosionFragments(float x, float y);
void spawnRainbowFragments(float x, float y, float t, float intensity = 1.0f);
void spawnBrushParticle(float x, float y, int brushEffectMode);
void spawnMeteorDrop(float x, float y);
void update_brush_painting(bool painting, int brushEffectMode, float centerX, float centerY, float& lastBrushX, float& lastBrushY, float& lastVelX, float& lastVelY);