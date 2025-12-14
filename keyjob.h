#pragma once
#include <SDL.h>
#include "GameConfig.h"
#include "SpatialGrid.h"
#include "ThreadPool.h"
#include "Render.h" 

void handle_input_events(
    SDL_Event& e,
    bool& running,
    bool& mouseDown,
    bool& brushMode,
    bool& painting,
    int& brushEffectMode,
    bool& showFPS,
    bool& silent,
    ThreadPool& pool,
    SpatialGrid& grid,
    SDL_Renderer* renderer,
    GameTextures& textures
);