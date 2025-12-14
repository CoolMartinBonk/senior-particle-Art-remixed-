#pragma once
#include <SDL.h>
#include <vector>
#include "GameConfig.h" 

struct GameTextures {
    SDL_Texture* normalParticle = nullptr;
    SDL_Texture* playerParticle = nullptr;
    SDL_Texture* playerGlow = nullptr;
    SDL_Texture* brush = nullptr;
    SDL_Texture* rainbowBrush = nullptr;
    SDL_Texture* dot = nullptr;
    SDL_Texture* metaballParticle = nullptr;
    SDL_Texture* metaballTarget = nullptr;
    SDL_Texture* plasmaTexture = nullptr;
};

SDL_Texture* create_brush_texture(SDL_Renderer* renderer, int size);
SDL_Texture* create_dot_texture(SDL_Renderer* renderer, int size);
SDL_Texture* create_metaball_particle_texture(SDL_Renderer* renderer, int size);
SDL_Texture* create_particle_texture(SDL_Renderer* renderer, int texture_size, SDL_Color color, bool is_glow);
SDL_Texture* create_rainbow_brush_texture(SDL_Renderer* renderer, int size);

void drawBoilingSunSurface(SDL_Renderer* renderer, SDL_Texture* texture, float cx, float cy, float radius, SDL_Color color, float time);

void recreate_all_textures(SDL_Renderer* renderer, GameTextures& tex, int width, int height);

void destroy_all_textures(GameTextures& tex);

void reset_alien_sky();

void render_frame(SDL_Renderer* renderer, const GameTextures& tex,
    bool brushMode, int brushEffectMode,
    bool playerSunMode, bool playerRainbow, float playerJumpTimer,
    bool showFPS, float fpsValue);

void render_fps_number(SDL_Renderer* renderer, float fps);
