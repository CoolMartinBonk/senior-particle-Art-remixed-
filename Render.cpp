#include "Render.h"
#include "GameLogic.h" 
#include <cmath>
#include <algorithm>
#include<random>
#undef min
#undef max

struct Star {
    float x, y;
    float baseSize;
    float phase;
    float r, g, b;
};

struct Nebula {
    float x, y;
    float scale;
    float angle;
    float rotSpeed;
    Uint8 r, g, b, a;
};

static std::vector<Star> stars;
static std::vector<Nebula> nebulas;

float smoothstep(float edge0, float edge1, float x) {
    float t = (std::max)(0.0f, (std::min)(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

void reset_alien_sky() {
    stars.clear();
    nebulas.clear();
}

void init_alien_sky(int w, int h) {
    if (!stars.empty()) return;

    for (int i = 0; i < 300; ++i) {
        float tier = (rand() % 100) / 100.0f;
        float size = (tier > 0.9f) ? 2.5f : ((tier > 0.6f) ? 1.5f : 0.8f);

        float tint = (rand() % 100) / 100.0f;
        float r, g, b;
        if (tint > 0.5f) {
            r = 150; g = 255; b = 255;
        }
        else {
            r = 255; g = 150; b = 255;
        }

        stars.push_back({
            (float)(rand() % w),
            (float)(rand() % h),
            size,
            (float)(rand() % 628) / 100.0f,
            r, g, b
            });
    }

    for (int i = 0; i < 6; ++i) {
        nebulas.push_back({
            (float)(rand() % w),
            (float)(rand() % h),
            4.0f + (rand() % 40) / 10.0f,
            (float)(rand() % 360),
            ((rand() % 100) - 50) / 2000.0f,
            (Uint8)(50 + rand() % 50),
            (Uint8)(20 + rand() % 30),
            (Uint8)(80 + rand() % 100),
            (Uint8)(30 + rand() % 40)
            });
    }
}

SDL_Texture* create_brush_texture(SDL_Renderer* renderer, int size) {
    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
    SDL_SetRenderTarget(renderer, tex);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    float cx = size / 2.0f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - cx, dy = y - cx;
            float dist = sqrtf(dx * dx + dy * dy) / (size / 2.0f);
            float alpha = 0.0f;
            if (dist < 1.0f) alpha = powf(1.0f - dist, 2.5f) * 180;
            if (dist < 0.4f) alpha += powf(1.0f - dist / 0.4f, 2.0f) * 80;
            SDL_SetRenderDrawColor(renderer, 120, 200, 255, (Uint8)alpha);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }
    SDL_SetRenderTarget(renderer, NULL);
    return tex;
}

SDL_Texture* create_dot_texture(SDL_Renderer* renderer, int size) {
    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
    SDL_SetRenderTarget(renderer, tex);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    float cx = size / 2.0f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - cx, dy = y - cx;
            float dist = sqrtf(dx * dx + dy * dy) / (size / 2.0f);
            if (dist < 1.0f) {
                float alpha = powf(1.0f - dist, 2.5f) * 255;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, (Uint8)alpha);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    SDL_SetRenderTarget(renderer, NULL);
    return tex;
}

SDL_Texture* create_metaball_particle_texture(SDL_Renderer* renderer, int size) {
    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetRenderTarget(renderer, tex);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    float c = size * 0.5f;
    float draw_r = c * 0.9f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - c + 0.5f, dy = y - c + 0.5f;
            float distSq = dx * dx + dy * dy;
            if (distSq < draw_r * draw_r) {
                float dist = sqrtf(distSq);
                float t = dist / draw_r;
                float alpha = expf(-t * t * 5.0f) * 255.0f;
                if (alpha > 255.0f) alpha = 255.0f;
                if (alpha > 1.0f) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, (Uint8)alpha);
                    SDL_RenderDrawPoint(renderer, x, y);
                }
            }
        }
    }
    SDL_SetRenderTarget(renderer, NULL);
    return tex;
}

SDL_Texture* create_particle_texture(SDL_Renderer* renderer, int texture_size, SDL_Color color, bool is_glow) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, texture_size, texture_size);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    float center = texture_size / 2.0f;
    for (int y = 0; y < texture_size; ++y) {
        for (int x = 0; x < texture_size; ++x) {
            float dist = sqrtf(powf(x - center, 2) + powf(y - center, 2));
            if (dist <= center) {
                float alpha_ratio = 1.0f - (dist / center);
                Uint8 a = is_glow ? (Uint8)(color.a * alpha_ratio * alpha_ratio) : (Uint8)(color.a * alpha_ratio);
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, a);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    SDL_SetRenderTarget(renderer, NULL);
    return texture;
}

SDL_Texture* create_rainbow_brush_texture(SDL_Renderer* renderer, int size) {
    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
    SDL_Texture* base_brush = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(base_brush, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, base_brush);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    float center = size / 2.0f;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dist_ratio = sqrtf(powf(x - center, 2) + powf(y - center, 2)) / center;
            if (dist_ratio <= 1.0f) {
                float alpha = powf(1.0f - dist_ratio, 2.5f) * 255.0f;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, (Uint8)alpha);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    SDL_SetRenderTarget(renderer, tex);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetTextureColorMod(base_brush, 120, 200, 255);
    SDL_SetTextureAlphaMod(base_brush, (Uint8)(255 * 0.7f));
    SDL_RenderCopy(renderer, base_brush, NULL, NULL);
    SDL_SetTextureColorMod(base_brush, 255, 255, 255);
    SDL_SetTextureAlphaMod(base_brush, (Uint8)(255 * 0.35f));
    SDL_Rect core_rect = { size / 4, size / 4, size / 2, size / 2 };
    SDL_RenderCopy(renderer, base_brush, NULL, &core_rect);
    SDL_DestroyTexture(base_brush);
    SDL_SetRenderTarget(renderer, NULL);
    return tex;
}

SDL_Texture* create_plasma_texture(SDL_Renderer* renderer, int size) {
    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size, size);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
    SDL_SetRenderTarget(renderer, tex);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    float cx = size / 2.0f;
    float maxR = size / 2.0f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - cx;
            float dy = y - cx;
            float dist = sqrtf(dx * dx + dy * dy);
            float t = dist / maxR;

            if (t >= 1.0f) continue;

            float glow1 = expf(-t * t * 4.0f) * 0.6f;
            float glow2 = expf(-t * t * 16.0f) * 0.8f;
            float core = expf(-t * t * 64.0f) * 1.0f;

            float brightness = glow1 + glow2 + core;
            if (brightness > 1.0f) brightness = 1.0f;

            Uint8 val = (Uint8)(brightness * 255);
            Uint8 alpha = (Uint8)(powf(1.0f - t, 0.5f) * 255 * brightness);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);
    return tex;
}

void recreate_all_textures(SDL_Renderer* renderer, GameTextures& tex, int width, int height) {
    destroy_all_textures(tex);
    tex.normalParticle = create_particle_texture(renderer, 16, { 100, 150, 200, 120 }, false);
    tex.playerParticle = create_particle_texture(renderer, 16, { 220, 240, 255, 255 }, false);
    tex.playerGlow = create_particle_texture(renderer, 32, { 80, 120, 200, 255 }, true);
    tex.brush = create_brush_texture(renderer, 128);
    tex.rainbowBrush = create_rainbow_brush_texture(renderer, 128);
    tex.dot = create_dot_texture(renderer, 16);
    tex.metaballParticle = create_metaball_particle_texture(renderer, 128);
    tex.plasmaTexture = create_plasma_texture(renderer, 64);

    int fluidW = (int)(width * FLUID_RENDER_SCALE);
    int fluidH = (int)(height * FLUID_RENDER_SCALE);
    tex.metaballTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, fluidW, fluidH);
    SDL_SetTextureBlendMode(tex.metaballTarget, SDL_BLENDMODE_BLEND);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
}

void destroy_all_textures(GameTextures& tex) {
    if (tex.plasmaTexture) SDL_DestroyTexture(tex.plasmaTexture);
    if (tex.normalParticle) SDL_DestroyTexture(tex.normalParticle);
    if (tex.playerParticle) SDL_DestroyTexture(tex.playerParticle);
    if (tex.playerGlow) SDL_DestroyTexture(tex.playerGlow);
    if (tex.brush) SDL_DestroyTexture(tex.brush);
    if (tex.rainbowBrush) SDL_DestroyTexture(tex.rainbowBrush);
    if (tex.dot) SDL_DestroyTexture(tex.dot);
    if (tex.metaballParticle) SDL_DestroyTexture(tex.metaballParticle);
    if (tex.metaballTarget) SDL_DestroyTexture(tex.metaballTarget);
    tex = GameTextures();
}

void drawBoilingSunSurface(SDL_Renderer* renderer, SDL_Texture* texture, float cx, float cy, float radius, SDL_Color color, float time) {
    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;

    const int SEGMENTS = 64;
    const float ANGLE_STEP = (2.0f * 3.14159f) / SEGMENTS;

    verts.push_back({ {cx, cy}, color, {0.5f, 0.5f} });

    for (int i = 0; i <= SEGMENTS; ++i) {
        float angle = i * ANGLE_STEP;
        float wave1 = 0.05f * sinf(angle * 6.0f + time * 2.0f);
        float wave2 = 0.03f * cosf(angle * 13.0f - time * 3.5f);
        float wave3 = 0.02f * sinf(angle * 20.0f + time * 5.0f);
        float dynamic_r = radius * (1.0f + wave1 + wave2 + wave3);
        float px = cx + cosf(angle) * dynamic_r;
        float py = cy + sinf(angle) * dynamic_r;
        float u = 0.5f + 0.5f * cosf(angle);
        float v = 0.5f + 0.5f * sinf(angle);
        verts.push_back({ {px, py}, color, {u, v} });
    }

    for (int i = 1; i <= SEGMENTS; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
    SDL_RenderGeometry(renderer, texture, verts.data(), (int)verts.size(), indices.data(), (int)indices.size());
}

void draw_alien_atmosphere(SDL_Renderer* renderer, int w, int h) {

    SDL_Color topColor = { 5, 10, 20, 255 };
    SDL_Color bottomColor = { 40, 10, 50, 255 };

    float energy = currentMusicEnergy;
    if (energy > 0.2f) {
        bottomColor.r = (Uint8)(40 + energy * 0.3);
        bottomColor.b = (Uint8)(50 + energy * 0.2);
    }

    SDL_Vertex verts[4];
    verts[0] = { {0, 0}, topColor, {0, 0} };
    verts[1] = { {(float)w, 0}, topColor, {1, 0} };
    verts[2] = { {0, (float)h}, bottomColor, {0, 1} };
    verts[3] = { {(float)w, (float)h}, bottomColor, {1, 1} };

    int indices[6] = { 0, 1, 2, 2, 1, 3 };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderGeometry(renderer, NULL, verts, 4, indices, 6);
}

void draw_giant_planet(SDL_Renderer* renderer, const GameTextures& tex, int w, int h, float time) {

    float energy = currentMusicEnergy;

    float px = w * 0.85f;
    float py = h * 0.2f;
    float radius = 200.0f + 10.0f * sinf(time * 0.5f);

    SDL_SetTextureBlendMode(tex.playerGlow, SDL_BLENDMODE_ADD);

    SDL_SetTextureColorMod(tex.playerGlow, 20, 80, 100);
    SDL_SetTextureAlphaMod(tex.playerGlow, 100);
    SDL_Rect r1 = { (int)(px - radius), (int)(py - radius), (int)(radius * 2), (int)(radius * 2) };
    SDL_RenderCopy(renderer, tex.playerGlow, NULL, &r1);

    float outerR = radius * (1.2f + energy * 0.2f);
    SDL_SetTextureColorMod(tex.playerGlow, 40, 20, 60);
    SDL_SetTextureAlphaMod(tex.playerGlow, (Uint8)(60 + energy * 100));
    SDL_Rect r2 = { (int)(px - outerR), (int)(py - outerR), (int)(outerR * 2), (int)(outerR * 2) };
    SDL_RenderCopy(renderer, tex.playerGlow, NULL, &r2);
}

void draw_nebula(SDL_Renderer* renderer, const GameTextures& tex, float time) {
    SDL_SetTextureBlendMode(tex.playerGlow, SDL_BLENDMODE_ADD);
    float energy = currentMusicEnergy;

    for (auto& n : nebulas) {
        n.angle += n.rotSpeed;

        float pulse = 1.0f + 0.1f * sinf(time + n.x);
        float musicScale = 1.0f + energy * 0.3f;
        float finalSize = 32.0f * n.scale * pulse * musicScale;

        SDL_SetTextureColorMod(tex.playerGlow, n.r, n.g, n.b);
        SDL_SetTextureAlphaMod(tex.playerGlow, n.a);

        SDL_Rect dst = {
            (int)(n.x - finalSize / 2),
            (int)(n.y - finalSize / 2),
            (int)finalSize,
            (int)finalSize
        };

        SDL_RenderCopyEx(renderer, tex.playerGlow, NULL, &dst, n.angle, NULL, SDL_FLIP_NONE);
    }
}

void draw_alien_sky_elements(SDL_Renderer* renderer, const GameTextures& tex, int w, int h, float time) {
    init_alien_sky(w, h);

    draw_alien_atmosphere(renderer, w, h);

    draw_giant_planet(renderer, tex, w, h, time);

    draw_nebula(renderer, tex, time);

    SDL_SetTextureBlendMode(tex.dot, SDL_BLENDMODE_ADD);

    float energy = currentMusicEnergy;
    static float smoothEnergy = 0.0f;
    smoothEnergy = smoothEnergy * 0.95f + energy * 0.05f;

    for (auto& s : stars) {

        float twinkle = 0.95f + 0.05f * sinf(time * 2.0f + s.phase);

        float brightness = twinkle * (0.7f + smoothEnergy * 0.6f);
        if (brightness > 1.0f) brightness = 1.0f;

        Uint8 alpha = (Uint8)(brightness * 255);

        SDL_SetTextureColorMod(tex.dot, (Uint8)s.r, (Uint8)s.g, (Uint8)s.b);
        SDL_SetTextureAlphaMod(tex.dot, alpha);

        float currentSize = s.baseSize * (1.0f + smoothEnergy * 0.1f);

        float renderSize = 4.0f * currentSize;

        SDL_Rect dst = {
            (int)(s.x - renderSize / 2),
            (int)(s.y - renderSize / 2),
            (int)renderSize,
            (int)renderSize
        };
        SDL_RenderCopy(renderer, tex.dot, NULL, &dst);
    }

    SDL_SetTextureColorMod(tex.dot, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex.dot, 255);
    SDL_SetTextureColorMod(tex.playerGlow, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex.playerGlow, 255);
}

void render_frame(SDL_Renderer* renderer, const GameTextures& tex, bool brushMode, int brushEffectMode, bool playerSunMode, bool playerRainbow, float playerJumpTimer, bool showFPS, float fpsValue) {

    float time = SDL_GetTicks() * 0.001f;

    SDL_SetRenderTarget(renderer, tex.metaballTarget);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    static std::vector<SDL_Vertex> metaballBatch;
    if (metaballBatch.capacity() < 6 * particles.size()) {
        metaballBatch.reserve(6 * particles.size());
    }
    metaballBatch.clear();

    static std::vector<SDL_Vertex> plasmaBatch;
    if (plasmaBatch.capacity() < 6 * particles.size()) {
        plasmaBatch.reserve(6 * particles.size());
    }
    plasmaBatch.clear();

    float waterBaseSize = RADIUS * METABALL_VISUAL_RADIUS_MULTIPLIER * FLUID_RENDER_SCALE;

    static float noiseLUT[1024];
    static bool initLUT = false;
    if (!initLUT) {
        for (int i = 0; i < 1024; ++i) noiseLUT[i] = (float)(rand() % 100) / 100.0f;
        initLUT = true;
    }

    for (const auto& p : particles) {
        if (p.isPlayer) continue;

        float temp = p.temperature;

        if (temp > 0.8f) {
            float heat = (temp - 0.8f) / 1.2f;
            if (heat > 1.0f) heat = 1.0f;

            int idx = ((int)p.x + (int)p.y) & 1023;
            float randomPhase = noiseLUT[idx] * 6.28f;
            float pulse = 1.0f + 0.15f * sinf(time * 25.0f + randomPhase);

            float size = RADIUS * 5.0f * (0.8f + 0.4f * heat) * pulse;
            float h = size * 0.5f;

            Uint8 r, g, b, a;
            if (heat < 0.3f) {
                float t = heat / 0.3f;
                r = (Uint8)(80 + 175 * t); g = 0; b = 0; a = (Uint8)(100 + 155 * t);
            }
            else if (heat < 0.7f) {
                float t = (heat - 0.3f) / 0.4f;
                r = 255; g = (Uint8)(180 * t); b = 0; a = 255;
            }
            else {
                float t = (heat - 0.7f) / 0.3f;
                r = 255; g = (Uint8)(180 + 40 * t); b = (Uint8)(100 * t); a = 255;
            }

            SDL_Color col = { r, g, b, a };

            plasmaBatch.push_back({ {p.x - h, p.y - h}, col, {0,0} });
            plasmaBatch.push_back({ {p.x + h, p.y - h}, col, {1,0} });
            plasmaBatch.push_back({ {p.x + h, p.y + h}, col, {1,1} });
            plasmaBatch.push_back({ {p.x - h, p.y - h}, col, {0,0} });
            plasmaBatch.push_back({ {p.x + h, p.y + h}, col, {1,1} });
            plasmaBatch.push_back({ {p.x - h, p.y + h}, col, {0,1} });
        }
        else {
            float drawX = p.x * FLUID_RENDER_SCALE;
            float drawY = p.y * FLUID_RENDER_SCALE;
            float halfBaseSize = waterBaseSize * 0.5f;

            float x0 = drawX - halfBaseSize; float y0 = drawY - halfBaseSize;
            float x1 = drawX + halfBaseSize; float y1 = drawY + halfBaseSize;

            SDL_Color waterColor = { 255, 255, 255, 255 };

            metaballBatch.push_back({ {x0, y0}, waterColor, {0, 0} });
            metaballBatch.push_back({ {x1, y0}, waterColor, {1, 0} });
            metaballBatch.push_back({ {x1, y1}, waterColor, {1, 1} });
            metaballBatch.push_back({ {x0, y0}, waterColor, {0, 0} });
            metaballBatch.push_back({ {x1, y1}, waterColor, {1, 1} });
            metaballBatch.push_back({ {x0, y1}, waterColor, {0, 1} });
        }
    }

    if (!metaballBatch.empty()) {
        static SDL_BlendMode particle_accumulate_mode = SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_SRC_ALPHA, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
            SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);

        SDL_SetTextureBlendMode(tex.metaballParticle, particle_accumulate_mode);
        SDL_SetTextureColorMod(tex.metaballParticle, 255, 255, 255);
        SDL_RenderGeometry(renderer, tex.metaballParticle, metaballBatch.data(), (int)metaballBatch.size(), NULL, 0);
    }

    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 15);
    SDL_RenderFillRect(renderer, NULL);

    draw_alien_sky_elements(renderer, tex, SCREEN_WIDTH, SCREEN_HEIGHT, time);

    SDL_SetTextureBlendMode(tex.metaballTarget, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(tex.metaballTarget, 40, 80, 180);
    SDL_SetTextureAlphaMod(tex.metaballTarget, 210);
    SDL_RenderCopy(renderer, tex.metaballTarget, NULL, NULL);

    SDL_SetTextureBlendMode(tex.metaballTarget, SDL_BLENDMODE_ADD);
    SDL_SetTextureColorMod(tex.metaballTarget, 0, 100, 255);
    SDL_SetTextureAlphaMod(tex.metaballTarget, 80);
    SDL_Rect glowRect = { -10, -10, SCREEN_WIDTH + 20, SCREEN_HEIGHT + 20 };
    SDL_RenderCopy(renderer, tex.metaballTarget, NULL, &glowRect);

    SDL_SetTextureColorMod(tex.metaballTarget, 255, 255, 255);
    SDL_SetTextureAlphaMod(tex.metaballTarget, 100);
    SDL_RenderCopy(renderer, tex.metaballTarget, NULL, NULL);
    SDL_SetTextureAlphaMod(tex.metaballTarget, 255);

    if (!plasmaBatch.empty()) {
        SDL_SetTextureBlendMode(tex.plasmaTexture, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorMod(tex.plasmaTexture, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex.plasmaTexture, 255);
        SDL_RenderGeometry(renderer, tex.plasmaTexture, plasmaBatch.data(), (int)plasmaBatch.size(), NULL, 0);
    }

    static std::vector<SDL_Vertex> rainbowBatch(MAX_RAINBOW_FRAGMENTS * 6);
    SDL_Vertex* vPtr = rainbowBatch.data();
    int vertCount = 0;

    for (const auto& rf : rainbowFragments) {
        if (rf.x < -50 || rf.x > SCREEN_WIDTH + 50 || rf.y < -50 || rf.y > SCREEN_HEIGHT + 50) continue;

        float life_progress = rf.t / rf.life;
        if (life_progress >= 1.0f) continue;

        Uint8 r, g, b, a;
        float current_size;

        if (rf.type == 1 || rf.type == 4) {
            float burn = 1.0f - life_progress;
            if (burn > 0.7f) { r = 255; g = 255; b = 220; }
            else if (burn > 0.4f) { r = 200; g = 50; b = 20; }
            else { r = 50; g = 50; b = 60; }
            a = (Uint8)(255.0f * burn);
            current_size = rf.size * (1.0f + life_progress * 2.0f);
        }
        else if (rf.type == 2) {
            float heat = 1.0f - life_progress;
            if (heat > 0.8f) { r = 255; g = 255; b = 220; a = 255; }
            else if (heat > 0.5f) { r = 255; g = (Uint8)(100 + (heat - 0.5f) / 0.3f * 155); b = 50; a = 240; }
            else if (heat > 0.2f) { r = (Uint8)(100 + (heat - 0.2f) / 0.3f * 155); g = 20; b = 10; a = (Uint8)(200 * (heat / 0.5f)); }
            else { r = 50; g = 50; b = 50; a = (Uint8)(100 * (heat / 0.2f)); }
            current_size = rf.size * (heat * heat * heat);
        }
        else if (rf.type == 3) {
            float fade = 1.0f - life_progress;
            r = 255;
            g = (Uint8)(150 * fade + 50);
            b = (Uint8)(50 * fade);
            a = (Uint8)(200 * fade);
            current_size = rf.size * fade;
        }
        else {
            float current_h = rf.h + life_progress * 0.5f;
            current_h = current_h - std::floor(current_h);
            int idx = static_cast<int>(current_h * RAINBOW_LUT_SIZE) & (RAINBOW_LUT_SIZE - 1);
            SDL_Color c = rainbowColorLUT[idx];
            r = c.r; g = c.g; b = c.b;
            a = static_cast<Uint8>(255.0f * (1.0f - life_progress) * rf.alpha0);
            current_size = rf.size * (1.0f - life_progress * 0.5f);
        }

        float half = current_size * 0.5f;
        float x1 = rf.x - half; float y1 = rf.y - half;
        float x2 = rf.x + half; float y2 = rf.y + half;

        SDL_Color col = { r, g, b, a };

        vPtr[vertCount++] = { {x1, y1}, col, {0, 0} };
        vPtr[vertCount++] = { {x2, y1}, col, {1, 0} };
        vPtr[vertCount++] = { {x1, y2}, col, {0, 1} };

        vPtr[vertCount++] = { {x2, y1}, col, {1, 0} };
        vPtr[vertCount++] = { {x2, y2}, col, {1, 1} };
        vPtr[vertCount++] = { {x1, y2}, col, {0, 1} };
    }

    if (vertCount > 0) {
        SDL_SetTextureBlendMode(tex.dot, SDL_BLENDMODE_ADD);
        SDL_RenderGeometry(renderer, tex.dot, rainbowBatch.data(), vertCount, nullptr, 0);
    }

    static std::vector<SDL_Vertex> blueBrushBatch;
    static std::vector<SDL_Vertex> rainbowBrushBatch;
    blueBrushBatch.clear();
    rainbowBrushBatch.clear();

    for (const auto& bp : brushParticles) {
        float size = bp.baseSize * (0.85f + 0.25f * sinf(bp.t * 1.2f + bp.phase));
        float alphaVal = (180 + 60 * sinf(bp.t * 1.7f + bp.phase)) * (1.0f + bp.impact * 0.18f);

        if (bp.dissolveFrame > 0) {
            float k = bp.dissolveFrame / 8.0f;
            if (k > 1.0f) k = 1.0f;
            size *= (1.0f + 1.2f * k);
            alphaVal *= (1.0f - k);
        }
        if (alphaVal > 255) alphaVal = 255;
        if (alphaVal < 0) alphaVal = 0;

        Uint8 alpha = (Uint8)alphaVal;
        Uint8 r = 255, g = 255, b = 255;

        bool useRainbowBatch = false;

        float drawY = bp.y;

        if (bp.type == BRUSH_BLUE) {
            r = 255; g = 255; b = 255;
            useRainbowBatch = false;
        }
        else if (bp.type == BRUSH_EXPLOSIVE) {
            float pulse = 0.5f + 0.5f * sinf(bp.t * 30.0f);
            r = 255; g = (Uint8)(50 * pulse); b = (Uint8)(50 * pulse);
            useRainbowBatch = false;
        }
        else if (bp.type == BRUSH_DROP) {

            float dist = bp.phase;

            if (dist < 15.0f) {
                useRainbowBatch = false;
                r = 255; g = 255; b = 255;
                alpha = 255;
                size = bp.baseSize * 1.2f;
            }
            else if (dist < 40.0f) {
                useRainbowBatch = true;
                int idx = static_cast<int>(fmodf(bp.t * 8.0f + dist * 5.0f, (float)RAINBOW_LUT_SIZE));
                SDL_Color c = rainbowColorLUT[idx];
                r = (Uint8)std::min(255, c.r + 120);
                g = (Uint8)std::min(255, c.g + 120);
                b = (Uint8)std::min(255, c.b + 120);
                alpha = 180;
                size = bp.baseSize * 1.5f;
            }
            else {
                useRainbowBatch = true;
                int idx = static_cast<int>(fmodf(bp.t * 12.0f + dist * 2.0f, (float)RAINBOW_LUT_SIZE));
                SDL_Color c = rainbowColorLUT[idx];
                r = c.r; g = c.g; b = c.b;
                float pulse = 1.0f + 0.05f * sinf(bp.t * 3.0f - dist * 0.1f);
                alpha = 60;
                size = bp.baseSize * 2.5f * pulse;
            }
        }
        else {
            int color_index = static_cast<int>(fmodf((bp.t + bp.phase) * 15.0f + bp.baseX * 0.1f, (float)RAINBOW_LUT_SIZE));
            SDL_Color c = rainbowColorLUT[color_index];
            r = c.r; g = c.g; b = c.b;
            useRainbowBatch = true;
        }

        float halfSize = size * 0.5f;
        float x0 = bp.x - halfSize; float y0 = drawY - halfSize;
        float x1 = bp.x + halfSize; float y1 = drawY + halfSize;

        SDL_Vertex vTL = { {x0, y0}, {r, g, b, alpha}, {0, 0} };
        SDL_Vertex vTR = { {x1, y0}, {r, g, b, alpha}, {1, 0} };
        SDL_Vertex vBR = { {x1, y1}, {r, g, b, alpha}, {1, 1} };
        SDL_Vertex vBL = { {x0, y1}, {r, g, b, alpha}, {0, 1} };

        if (useRainbowBatch) {
            rainbowBrushBatch.push_back(vTL); rainbowBrushBatch.push_back(vTR); rainbowBrushBatch.push_back(vBL);
            rainbowBrushBatch.push_back(vBL); rainbowBrushBatch.push_back(vTR); rainbowBrushBatch.push_back(vBR);
        }
        else {
            blueBrushBatch.push_back(vTL); blueBrushBatch.push_back(vTR); blueBrushBatch.push_back(vBL);
            blueBrushBatch.push_back(vBL); blueBrushBatch.push_back(vTR); blueBrushBatch.push_back(vBR);
        }
    }

    if (!blueBrushBatch.empty()) {
        SDL_SetTextureBlendMode(tex.brush, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorMod(tex.brush, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex.brush, 255);
        SDL_RenderGeometry(renderer, tex.brush, blueBrushBatch.data(), (int)blueBrushBatch.size(), NULL, 0);
    }
    if (!rainbowBrushBatch.empty()) {
        SDL_SetTextureBlendMode(tex.rainbowBrush, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorMod(tex.rainbowBrush, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex.rainbowBrush, 255);
        SDL_RenderGeometry(renderer, tex.rainbowBrush, rainbowBrushBatch.data(), (int)rainbowBrushBatch.size(), NULL, 0);
    }

    if (brushMode) {
        SDL_SetRenderDrawColor(renderer, 200, 255, 255, 180);
        SDL_Rect hint = { SCREEN_WIDTH - 220, 20, 200, 36 };
        SDL_RenderFillRect(renderer, &hint);
    }

    static std::vector<const Particle*> drawOrder;
    drawOrder.resize(particles.size());
    for (size_t i = 0; i < particles.size(); ++i) {
        drawOrder[i] = &particles[i];
    }

    std::sort(drawOrder.begin(), drawOrder.end(), [](const Particle* a, const Particle* b) {
        return a->id < b->id;
        });

    SDL_SetTextureBlendMode(tex.playerParticle, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(tex.playerGlow, SDL_BLENDMODE_ADD);

    for (const Particle* p_ptr : drawOrder) {
        const auto& p = *p_ptr;
        if (!p.isPlayer) continue;

        Uint8 r = 255, g = 255, b = 255, a = 255;
        float scale = 1.0f;
        bool useAddMode = false;

        if (brushMode) {
            if (brushEffectMode == 1) { r = 255; g = 255; b = 255; }
            else if (brushEffectMode == 2) { float h = fmodf(SDL_GetTicks() * 0.0005f + p.id * 0.01f, 1.0f); HSVtoRGB(h, 0.8f, 1.0f, r, g, b); }
            else if (brushEffectMode == 3) { r = 255; g = 100; b = 50; useAddMode = true; }
        }
        else {
            if (playerSunMode) {
                r = 255; g = 80; b = 20; useAddMode = true;
                int particle_count = 1;
                float current_render_radius = RADIUS;
                float spdSq = p.vx * p.vx + p.vy * p.vy;
                float dirX = 0, dirY = 1;
                float speedVal = 0;
                if (spdSq > 0.01f) { speedVal = sqrtf(spdSq); dirX = p.vx / speedVal; dirY = p.vy / speedVal; }

                static int frameCounter = 0;
                frameCounter++;
                if (frameCounter % 4 == 0) {
                    for (int k = 0; k < particle_count; ++k) {
                        RainbowFragment spark;
                        float perpX = -dirY; float perpY = dirX;
                        float rnd1 = (rand() % 100) / 100.0f; float rnd2 = (rand() % 100) / 100.0f;
                        float gaussian = (rnd1 + rnd2 - 1.0f);
                        float thickness = current_render_radius * 1.2f;
                        float offsetX = perpX * gaussian * thickness;
                        float offsetY = perpY * gaussian * thickness;
                        float lag = ((rand() % 100) / 100.0f) * current_render_radius * 0.8f;
                        spark.x = p.x + offsetX - dirX * lag;
                        spark.y = p.y + offsetY - dirY * lag;
                        if (speedVal < 0.1f) { float a = (rand() % 628) / 100.0f; spark.vx = cosf(a) * 1.0f; spark.vy = sinf(a) * 1.0f; }
                        else { spark.vx = p.vx * 0.8f - dirX * 1.5f; spark.vy = p.vy * 0.8f - dirY * 1.5f; }
                        spark.life = 1.5f;
                        spark.size = current_render_radius * 2.5f;
                        spark.t = 0;
                        spark.type = 3;
                        spark.alpha0 = 0.6f;
                        rainbowFragments.push_back(spark);
                    }
                }
            }
            else if (playerRainbow) {
                float h = fmodf(SDL_GetTicks() * 0.0005f + p.id * 0.01f, 1.0f);
                HSVtoRGB(h, 0.8f, 1.0f, r, g, b);
            }
        }

        float current_render_radius = brushMode ? 6.0f : RADIUS;
        float px = p.x;
        float py = p.y;

        if (!brushMode && playerRainbow && playerJumpTimer > 0) {
            float jump = sinf(SDL_GetTicks() / 30.0f + p.id) * 8.0f * (playerJumpTimer / 0.5f);
            px += cosf((float)p.id) * jump;
            py += sinf((float)p.id) * jump;
            scale = 1.0f + 0.2f * (playerJumpTimer / 0.5f);
        }

        SDL_SetTextureColorMod(tex.playerGlow, r, g, b);
        SDL_SetTextureColorMod(tex.playerParticle, r, g, b);

        if (useAddMode) {
            SDL_SetTextureBlendMode(tex.playerParticle, SDL_BLENDMODE_ADD);
            SDL_SetTextureAlphaMod(tex.playerParticle, 180);
        }
        else {
            SDL_SetTextureBlendMode(tex.playerParticle, SDL_BLENDMODE_BLEND);
            SDL_SetTextureAlphaMod(tex.playerParticle, 255);
        }

        float glow_radius = current_render_radius * 1.6f * scale;
        SDL_Rect glow_dest = { (int)(px - glow_radius), (int)(py - glow_radius), (int)(glow_radius * 2), (int)(glow_radius * 2) };
        SDL_RenderCopy(renderer, tex.playerGlow, NULL, &glow_dest);

        float core_radius = current_render_radius * scale;
        SDL_Rect core_dest = { (int)(px - core_radius), (int)(py - core_radius), (int)(core_radius * 2), (int)(core_radius * 2) };
        SDL_RenderCopy(renderer, tex.playerParticle, NULL, &core_dest);
    }

    if (showFPS) {
        render_fps_number(renderer, fpsValue);
    }
}

static void draw_segment_digit(SDL_Renderer* renderer, int x, int y, int digit, float scale) {
    int w = (int)(10 * scale);
    int h = (int)(10 * scale);
    int h2 = h * 2;

    struct Seg { int x1, y1, x2, y2; };
    Seg segs[7] = {
        {0,0, w,0}, {0,0, 0,h}, {w,0, w,h}, {0,h, w,h}, {0,h, 0,h2}, {w,h, w,h2}, {0,h2, w,h2}
    };
    static const int map[10][7] = {
        {1,1,1,0,1,1,1}, {0,0,1,0,0,1,0}, {1,0,1,1,1,0,1}, {1,0,1,1,0,1,1}, {0,1,1,1,0,1,0},
        {1,1,0,1,0,1,1}, {1,1,0,1,1,1,1}, {1,0,1,0,0,1,0}, {1,1,1,1,1,1,1}, {1,1,1,1,0,1,1}
    };

    for (int i = 0; i < 7; ++i) {
        if (map[digit][i]) SDL_RenderDrawLine(renderer, x + segs[i].x1, y + segs[i].y1, x + segs[i].x2, y + segs[i].y2);
    }
}

void render_fps_number(SDL_Renderer* renderer, float fps) {
    SDL_SetRenderTarget(renderer, NULL);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    int val = (int)fps;
    if (val < 0) val = 0;
    if (val > 999) val = 999;

    std::vector<int> digits;
    if (val == 0) digits.push_back(0);
    while (val > 0) { digits.push_back(val % 10); val /= 10; }

    int x = 20;
    int y = 20;
    float scale = 3.0f;

    for (int i = (int)digits.size() - 1; i >= 0; --i) {
        draw_segment_digit(renderer, x, y, digits[i], scale);
        x += (int)(18 * scale);
    }
}