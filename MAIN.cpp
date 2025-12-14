#define MA_ENABLE_DECODERS
#define MINIAUDIO_IMPLEMENTATION
#define NOMINMAX

#include <random>
#include "GameConfig.h"
#include "SpatialGrid.h"
#include "ThreadPool.h"
#include "AudioSystem.h"
#include "PhysicsSystem.h"
#include "Render.h"
#include "GameLogic.h"
#include "keyjob.h"
#include"Simulation.h"

int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;
float currentMusicEnergy = 0.0f;

std::vector<SDL_Color> rainbowColorLUT(RAINBOW_LUT_SIZE);
std::vector<Particle> particles;
std::vector<Particle> particle_buffer;
std::vector<Vector2D> forces;
std::vector<RainbowFragment> rainbowFragments;
std::vector<BrushParticle> brushParticles;
std::vector<float> density_buffer;
int density_buffer_width = 0;
int density_buffer_height = 0;

SynthSound blueSound;
SynthSound rainbowSound;
SynthSound explosionSound;

std::vector<SynthSound*> sounds_to_play;
std::mutex sounds_to_play_mutex;
SDL_AudioDeviceID audioDevice = 0;

thread_local std::minstd_rand rng_sampler;

void calculate_player_cohesion_forces(std::vector<Vector2D>& forces);
void calculate_mouse_interaction_forces(int mx, int my, bool leftDown, std::vector<Vector2D>& forces, bool playerSunMode);
void apply_forces_to_particles(std::vector<Vector2D>& forces);
void resolve_brush_collisions(bool brushMode, float avgVx, float avgVy, bool& playerRainbow, float& playerRainbowTimer, float& playerJumpTimer);
void apply_heat_from_fragments(const SpatialGrid& grid);

int main(int argc, char* argv[]) {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    unsigned int n_threads = std::thread::hardware_concurrency();
    if (n_threads == 0) n_threads = 4;
    ThreadPool pool(n_threads);
    printf("Using %u threads.\n", n_threads);

    SDL_AudioSpec want = {}, have = {};
    want.freq = 44100; want.format = AUDIO_F32SYS; want.channels = 1; want.samples = 1024; want.callback = audio_callback;
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    make_blue_sound(blueSound); make_rainbow_sound(rainbowSound); make_stellar_explosion_sound(explosionSound);
    SDL_PauseAudioDevice(audioDevice, 0);

    SDL_Window* window = SDL_CreateWindow("Gift From Other planet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    ma_result result; ma_engine engine; ma_engine_init(NULL, &engine);
    ma_sound background_music;
    ma_sound_init_from_file(&engine, "Stellardrone - Eternity.mp3", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_STREAM, NULL, NULL, &background_music);
    ma_sound_set_looping(&background_music, MA_TRUE);
    ma_sound_start(&background_music);

    srand((unsigned int)time(0));
    generateRainbowLUT();

    GameTextures textures;
    recreate_all_textures(renderer, textures, SCREEN_WIDTH, SCREEN_HEIGHT);

    density_buffer_width = SCREEN_WIDTH / DENSITY_BUFFER_SCALE;
    density_buffer_height = SCREEN_HEIGHT / DENSITY_BUFFER_SCALE;
    density_buffer.resize(density_buffer_width * density_buffer_height);

    SpatialGrid grid(SCREEN_WIDTH, SCREEN_HEIGHT, INTERACTION_RADIUS);

    particles.reserve(TOTAL_PARTICLES);
    forces.resize(TOTAL_PARTICLES);
    int global_index = 0;

    for (int i = 0; i < PLAYER_PARTICLE_COUNT; ++i) {
        float angle = (float)i / PLAYER_PARTICLE_COUNT * 2.0f * 3.14159f;
        float r = (rand() % 40);
        particles.emplace_back(SCREEN_WIDTH / 2 + cos(angle) * r, SCREEN_HEIGHT / 2 + sin(angle) * r, true);
        particles.back().id = global_index++;
    }

    for (int i = 0; i < TOTAL_PARTICLES - PLAYER_PARTICLE_COUNT; ++i) {
        particles.emplace_back(rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, false);
        particles.back().id = global_index++;
    }

    bool running = true;
    bool mouseDown = false;
    int mx = 0, my = 0;
    bool brushMode = false;
    bool painting = false;
    float lastBrushX = SCREEN_WIDTH / 2, lastBrushY = SCREEN_HEIGHT / 2;
    float lastVelX = 0.0f, lastVelY = 0.0f;
    int brushEffectMode = 1;
    bool playerSunMode = false;
    float playerSunTimer = 0.0f;
    bool playerRainbow = false;
    float playerRainbowTimer = 0.0f;
    float playerJumpTimer = 0.0f;
    float meteorTimer = 0.0f;
    float nextMeteorInterval = 2.0f + (rand() % 300) / 100.0f;
    float centerX = 0.0f, centerY = 0.0f, avgVx = 0.0f, avgVy = 0.0f;
    int TARGET_FPS = 90;
    const int FRAME_DELAY = 1000 / TARGET_FPS;
    Uint32 frameStart;
    int frameTime;
    Uint32 fpsLastTime = 0;
    int fpsFrames = 0;
    float finalFPS = 0.0f;
    bool showFPS = false;
    bool silent = true;

    while (running) {
        frameStart = SDL_GetTicks();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            handle_input_events(e, running, mouseDown, brushMode, painting, brushEffectMode, showFPS, silent, pool, grid, renderer, textures);
        }
            SDL_GetMouseState(&mx, &my);

            update_physics_simulation(brushMode, mx, my, mouseDown, playerSunMode, playerRainbow,centerX, centerY, avgVx, avgVy, playerRainbowTimer,playerJumpTimer,grid, pool);
            update_meteors(meteorTimer, nextMeteorInterval, silent, TARGET_FPS);
            update_brush_painting(brushMode && painting, brushEffectMode, centerX, centerY, lastBrushX, lastBrushY, lastVelX, lastVelY);
            update_brush_particles(brushMode, playerSunMode, playerSunTimer);
            update_rainbow_fragments();

            if (playerRainbow) { playerRainbowTimer -= 0.016f; playerJumpTimer -= 0.016f; if (playerRainbowTimer < 0) playerRainbow = false; }
            if (playerSunMode) { playerSunTimer -= 0.016f; if (playerSunTimer <= 0) playerSunMode = false; }

            fpsFrames++;
            if (SDL_GetTicks() - fpsLastTime >= 1000) {
                finalFPS = fpsFrames * 1000.0f / (SDL_GetTicks() - fpsLastTime);
                fpsFrames = 0;
                fpsLastTime = SDL_GetTicks();
            }

            render_frame(renderer, textures, brushMode, brushEffectMode,
                playerSunMode, playerRainbow, playerJumpTimer,
                showFPS, finalFPS);
            SDL_RenderPresent(renderer);

            frameTime = SDL_GetTicks() - frameStart;
            if (FRAME_DELAY > frameTime) {
                SDL_Delay(FRAME_DELAY - frameTime);
            }
        }
    destroy_all_textures(textures);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(audioDevice);
    ma_sound_uninit(&background_music);
    ma_engine_uninit(&engine);
    SDL_Quit();
    return 0;
}