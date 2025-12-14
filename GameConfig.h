#pragma once
#include "miniaudio.h"
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <vector>
#include <mutex>
#include <cmath>
#include <algorithm>

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern float currentMusicEnergy;

const float PHI = 1.61803398875f;
const int TOTAL_PARTICLES = 2000;
const int PLAYER_PARTICLE_COUNT = 300;
const float RADIUS = 20.0f;
const float DAMPING = 0.983f;
const float GRAVITY = 0.05f;
const float BOILING_POINT = 0.8f;
const float INTERACTION_RADIUS = 20.0f;
const float PLAYER_WATER_INTERACTION_RADIUS = 40.0f;
const float REPULSION_FORCE = 0.5f;
const float MOUSE_FORCE = 0.2f;
const float COHESION_FORCE = 0.02f;
const float PLAYER_WATER_REPULSION_MULTIPLIER = 8.0f;
const float METABALL_VISUAL_RADIUS_MULTIPLIER = 5.0f;
const float MAX_DISTURB = 200.0f;
const float MAX_STEP = MAX_DISTURB;
const size_t MAX_BRUSH_PARTICLES = 10000;
const size_t BRUSH_CULL_BATCH = 200;
static const float SYNTH_MASTER_GAIN = 0.0618f;
const float T = 432.0f / 440.0f;
const int DENSITY_BUFFER_SCALE = 8;
const int RAINBOW_LUT_SIZE = 512;
const int MAX_RAINBOW_FRAGMENTS = 20000;
const float FLUID_RENDER_SCALE = 0.5f;

enum BrushType { BRUSH_BLUE = 1, BRUSH_RAINBOW = 2, BRUSH_EXPLOSIVE = 3, BRUSH_DROP = 4};

struct BrushParticle {
    float x, y, baseX, baseY, baseSize, t, phase, impact = 0.0f, vx = 0.0f, vy = 0.0f;
    int highImpactFrames = 0, dissolveFrame = 0;
    BrushType type = BRUSH_BLUE;
    bool absorbed = false;
    bool hasWater = false;
};

struct RainbowFragment {
    float x, y, vx, vy, t, life, size, angle, spiralSpeed, h, alpha0;
    int type;
};

struct SynthSound {
    std::vector<float> samples;
    std::vector<int> playheads;
    bool playing = false;
};

struct Particle {
    float x, y, vx, vy;
    bool isPlayer;
    float temperature = 0.0f;
    int id;

    Particle(float px = 0, float py = 0, bool player = false)
        : x(px), y(py), vx(0), vy(0), isPlayer(player), temperature(0.0f), id(0) {}
};

struct Vector2D {
    float fx = 0.0f;
    float fy = 0.0f;
};

extern std::vector<SDL_Color> rainbowColorLUT;
extern std::vector<Particle> particles;
extern std::vector<Particle> particle_buffer;
extern std::vector<Vector2D> forces;
extern std::vector<RainbowFragment> rainbowFragments;
extern std::vector<BrushParticle> brushParticles;
extern std::vector<float> density_buffer;
extern int density_buffer_width, density_buffer_height;
extern SynthSound blueSound, rainbowSound, explosionSound;
extern std::mutex sounds_to_play_mutex;
extern std::vector<SynthSound*> sounds_to_play;
extern SDL_AudioDeviceID audioDevice;

void calculate_forces_for_keys(const std::vector<int>& cell_indices, const class SpatialGrid& grid, std::vector<Vector2D>& local_forces);
void make_rainbow_sound(SynthSound& snd);
void make_stellar_explosion_sound(SynthSound& snd);
void make_blue_sound(SynthSound& snd);
void request_play(SynthSound& snd);
void audio_callback(void* userdata, Uint8* stream, int len);