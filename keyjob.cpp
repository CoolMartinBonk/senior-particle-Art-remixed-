#include "keyjob.h"

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
) {
    if (e.type == SDL_QUIT) {
        running = false;
    }

    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
        pool.wait();

        float oldW = (float)SCREEN_WIDTH;
        float oldH = (float)SCREEN_HEIGHT;

        SCREEN_WIDTH = e.window.data1;
        SCREEN_HEIGHT = e.window.data2;

        for (auto& p : particles) {
            p.x = (p.x / oldW) * SCREEN_WIDTH;
            p.y = (p.y / oldH) * SCREEN_HEIGHT;
            p.vx = 0; p.vy = 0;
        }

        grid.resize((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
        density_buffer_width = SCREEN_WIDTH / DENSITY_BUFFER_SCALE;
        density_buffer_height = SCREEN_HEIGHT / DENSITY_BUFFER_SCALE;
        density_buffer.resize(density_buffer_width * density_buffer_height);

        recreate_all_textures(renderer, textures, SCREEN_WIDTH, SCREEN_HEIGHT);
        reset_alien_sky();
    }

    if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_LEFT) {
            mouseDown = true;
            if (brushMode) painting = true;
        }
        if (e.button.button == SDL_BUTTON_RIGHT) {
            brushMode = !brushMode;
            painting = false;
            SDL_ShowCursor(brushMode ? SDL_DISABLE : SDL_ENABLE);
        }
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        mouseDown = false;
        if (brushMode) painting = false;
    }

    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_1) brushEffectMode = 1;
        if (e.key.keysym.sym == SDLK_2) brushEffectMode = 2;

        if (!silent) {
            if (e.key.keysym.sym == SDLK_3) brushEffectMode = 3;
        }

        if (e.key.keysym.sym == SDLK_F3) {
            if (e.key.repeat == 0) {
                showFPS = !showFPS;
            }
        }

        if (e.key.keysym.sym == SDLK_F4) {
            if (e.key.repeat == 0) {
                silent = !silent;
            }
        }
    }
}