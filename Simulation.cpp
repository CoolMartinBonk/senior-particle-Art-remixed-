#include "Simulation.h"
#include "PhysicsSystem.h"
#include "GameLogic.h"

void update_physics_simulation(bool brushMode, int mx, int my, bool mouseDown, bool playerSunMode, bool playerRainbow, float& centerX, float& centerY, float& avgVx, float& avgVy,float& playerRainbowTimer,float& playerJumpTimer, SpatialGrid& grid, ThreadPool& pool) {

    int pCount = 0;
    for (auto& p : particles) if (p.isPlayer) { centerX += p.x; centerY += p.y; avgVx += p.vx; avgVy += p.vy; pCount++; }
    if (pCount > 0) { centerX /= pCount; centerY /= pCount; avgVx /= pCount; avgVy /= pCount; }

    if (brushMode) {
        for (size_t i = 0; i < particles.size(); ++i) {
            if (particles[i].isPlayer) {
                float angle = particles[i].id * 6.28f / PLAYER_PARTICLE_COUNT;
                particles[i].x = mx + cos(angle) * 35.0f;
                particles[i].y = my + sin(angle) * 35.0f;
                particles[i].vx = 0; particles[i].vy = 0;
            }
        }
        centerX = (float)mx; centerY = (float)my;
    }
    else {
        std::fill(forces.begin(), forces.end(), Vector2D());
        grid.update_and_sort(particles, particle_buffer);
        std::fill(density_buffer.begin(), density_buffer.end(), 0.0f);
        for (const auto& p : particles) {
            int bx = (int)(p.x / DENSITY_BUFFER_SCALE), by = (int)(p.y / DENSITY_BUFFER_SCALE);
            if (bx >= 0 && bx < density_buffer_width && by >= 0 && by < density_buffer_height) density_buffer[by * density_buffer_width + bx] += 1.0f;
        }
        pool.dispatch_repulsion_calc(grid.get_active_keys(), grid);
        pool.wait();
        pool.reduce_forces(forces);
        apply_heat_from_fragments(grid);
        calculate_mouse_interaction_forces(mx, my, mouseDown, forces, playerSunMode);
        calculate_player_cohesion_forces(forces);
        apply_forces_to_particles(forces);
        resolve_brush_collisions(brushMode, avgVx, avgVy, playerRainbow, playerRainbowTimer, playerJumpTimer);
    }
}

void update_meteors(float& timer, float& interval, bool silent, int& TARGET_FPS) {
    if (silent) return;

    timer += 1.0f / TARGET_FPS;

    if (timer > interval) {
        int batchSize = 1;
        int r = rand() % 100;
        if (r > 70) batchSize = 2;
        if (r > 90) batchSize = 3;

        for (int k = 0; k < batchSize; ++k) {
            float dropX = 100.0f + (rand() % (SCREEN_WIDTH - 200));
            float dropY = -50.0f - (rand() % 200);
            spawnMeteorDrop(dropX, dropY);
        }

        timer = 0.0f;
        interval = 0.5f + (rand() % 400) / 100.0f;
    }
}