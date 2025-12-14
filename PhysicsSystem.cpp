#include "GameConfig.h"
#include "PhysicsSystem.h"
#include "SpatialGrid.h"
#include "AudioSystem.h"
#include "GameLogic.h"  

void calculate_forces_for_keys(const std::vector<int>& cell_indices, const SpatialGrid& grid, std::vector<Vector2D>& local_forces) {
    const int cols = grid.cols;
    const int rows = grid.rows;
    const float R_INTERACT_SQ = INTERACTION_RADIUS * INTERACTION_RADIUS;
    const float R_PLAYER_SQ = PLAYER_WATER_INTERACTION_RADIUS * PLAYER_WATER_INTERACTION_RADIUS;
    const float COEFF_NORM = REPULSION_FORCE * 0.01f;
    const float COEFF_PLAYER = COEFF_NORM * PLAYER_WATER_REPULSION_MULTIPLIER;

    for (int idx : cell_indices) {

        int start1 = grid.cellStart[idx];
        int count1 = grid.cellCount[idx];
        int end1 = start1 + count1;

        if (count1 == 0) continue;

        int cy = idx / cols;
        int cx = idx % cols;

        for (int i = start1; i < end1; ++i) {
            Particle& p1 = particles[i];

            for (int ny = cy - 1; ny <= cy + 1; ++ny) {
                if (ny < 0 || ny >= rows) continue;
                int y_offset = ny * cols;

                for (int nx = cx - 1; nx <= cx + 1; ++nx) {
                    if (nx < 0 || nx >= cols) continue;

                    int neighbor_cell_idx = nx + y_offset;

                    int start2 = grid.cellStart[neighbor_cell_idx];
                    int count2 = grid.cellCount[neighbor_cell_idx];
                    int end2 = start2 + count2;

                    if (count2 == 0) continue;

                    for (int j = start2; j < end2; ++j) {
  
                        if (i >= j) continue;

                        Particle& p2 = particles[j];

                        float dx = p2.x - p1.x;
                        float dy = p2.y - p1.y;
                        float dist2 = dx * dx + dy * dy;

                        bool diffType = (p1.isPlayer != p2.isPlayer);
                        float rSq = diffType ? R_PLAYER_SQ : R_INTERACT_SQ;

                        if (dist2 < rSq && dist2 > 0.001f) {

                            float dist = std::sqrt(dist2);
                            float invDist = 1.0f / dist;

                            float radius = diffType ? PLAYER_WATER_INTERACTION_RADIUS : INTERACTION_RADIUS;
                            float f = (radius - dist) * (diffType ? COEFF_PLAYER : COEFF_NORM);

                            float scalar = f * invDist;
                            float pushX = dx * scalar;
                            float pushY = dy * scalar;

                            local_forces[i].fx -= pushX;
                            local_forces[i].fy -= pushY;
                            local_forces[j].fx += pushX;
                            local_forces[j].fy += pushY;
                        }
                    }
                }
            }

            if (!p1.isPlayer) {
                int bx = (int)(p1.x / DENSITY_BUFFER_SCALE);
                int by = (int)(p1.y / DENSITY_BUFFER_SCALE);

                if (bx > 0 && bx < density_buffer_width - 1 && by > 0 && by < density_buffer_height - 1) {
                    float dens = density_buffer[by * density_buffer_width + bx];

                    if (dens > 12.0f) {
                        const int K_SAMPLES = 24;
                        float sx = 0.0f, sy = 0.0f;
                        int sc = 0;

                        for (int ny = cy - 1; ny <= cy + 1 && sc < K_SAMPLES; ++ny) {
                            if (ny < 0 || ny >= rows) continue;
                            for (int nx = cx - 1; nx <= cx + 1 && sc < K_SAMPLES; ++nx) {
                                if (nx < 0 || nx >= cols) continue;
                                int nidx = nx + ny * cols;

                                int n_start = grid.cellStart[nidx];
                                int n_end = n_start + grid.cellCount[nidx];

                                for (int k = n_start; k < n_end && sc < K_SAMPLES; ++k) {
                                    if (k != i && !particles[k].isPlayer) {
                                        float dx = particles[k].x - p1.x;
                                        float dy = particles[k].y - p1.y;
                                        if (dx * dx + dy * dy < 2500.0f) {
                                            sx += particles[k].x;
                                            sy += particles[k].y;
                                            sc++;
                                        }
                                    }
                                }
                            }
                        }

                        if (sc > 0) {
                            float cx_val = sx / sc;
                            float cy_val = sy / sc;
                            local_forces[i].fx += (p1.x - cx_val) * 0.025f;
                            local_forces[i].fy += (p1.y - cy_val) * 0.025f;
                        }
                    }
                    else {
                        float gx = density_buffer[by * density_buffer_width + bx + 1] - density_buffer[by * density_buffer_width + bx - 1];
                        float gy = density_buffer[(by + 1) * density_buffer_width + bx] - density_buffer[(by - 1) * density_buffer_width + bx];
                        local_forces[i].fx -= gx * 0.0005f;
                        local_forces[i].fy -= gy * 0.0005f;
                    }
                }
            }
        }
    }
}

void calculate_player_cohesion_forces(std::vector<Vector2D>& forces) {
    float centerX = 0.0f;
    float centerY = 0.0f;
    int count = 0;
    for (size_t i = 0; i < particles.size(); ++i) {
        if (particles[i].isPlayer) {
            centerX += particles[i].x;
            centerY += particles[i].y;
            count++;
        }
    }
    if (count == 0) {
        return;
    }
    centerX /= count;
    centerY /= count;
    for (size_t i = 0; i < particles.size(); ++i) {
        if (particles[i].isPlayer) {
            float dx = centerX - particles[i].x;
            float dy = centerY - particles[i].y;
            forces[i].fx += dx * COHESION_FORCE;
            forces[i].fy += dy * COHESION_FORCE;
        }
    }
}

void calculate_mouse_interaction_forces(int mx, int my, bool leftDown, std::vector<Vector2D>& forces, bool playerSunMode) {
    if (!leftDown) {
        return;
    }
    for (size_t i = 0; i < particles.size(); ++i) {
        if (particles[i].isPlayer) {
            float dx = mx - particles[i].x;
            float dy = my - particles[i].y;
            if (playerSunMode) {
                forces[i].fx += dx * 0.015f * MOUSE_FORCE;
                forces[i].fy += dy * 0.015f * MOUSE_FORCE;
            }
            else {
                forces[i].fx += dx * 0.01f * MOUSE_FORCE;
                forces[i].fy += dy * 0.01f * MOUSE_FORCE;
            }
        }
    }
}

void apply_forces_to_particles(std::vector<Vector2D>& forces) {
    for (size_t i = 0; i < particles.size(); ++i) {

        if (particles[i].x < -5000.0f) continue;

        if (!particles[i].isPlayer) {
            if (particles[i].temperature > 0.0f) {
                particles[i].temperature -= 0.009f;
                if (particles[i].temperature < 0.0f) particles[i].temperature = 0.0f;
            }

            float temp = particles[i].temperature;

            if (temp > 0.8f) {
                particles[i].vx *= 0.90f;
                particles[i].vy *= 0.90f;

                float jitterStrength = (temp - 0.8f) * 0.5f;
                particles[i].vx += ((rand() % 100) / 50.0f - 1.0f) * jitterStrength;
                particles[i].vy += ((rand() % 100) / 50.0f - 1.0f) * jitterStrength;
            }
            else {
                particles[i].vy += GRAVITY;
            }
        }

        particles[i].vx += forces[i].fx;
        particles[i].vy += forces[i].fy;

        if (particles[i].isPlayer) {
            particles[i].vx *= DAMPING;
            particles[i].vy *= DAMPING;
        }
        else if (particles[i].temperature <= 0.8f) {
            particles[i].vx *= DAMPING;
            particles[i].vy *= DAMPING;
        }

        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;

        float jitter = (rand() & 15) * 0.01f;
        if (particles[i].x < RADIUS) {
            particles[i].x = RADIUS + jitter;
            particles[i].vx *= -0.5f;
        }
        if (particles[i].x > SCREEN_WIDTH - RADIUS) {
            particles[i].x = SCREEN_WIDTH - RADIUS - jitter;
            particles[i].vx *= -0.5f;
        }
        if (particles[i].y < RADIUS) {
            particles[i].y = RADIUS + jitter;
            particles[i].vy *= -0.5f;
        }
        if (particles[i].y > SCREEN_HEIGHT - RADIUS) {
            particles[i].y = SCREEN_HEIGHT - RADIUS - jitter;
            particles[i].vy *= -0.5f;
        }
    }
}

static const float BRUSH_GRID_CELL_SIZE = 100.0f;
static std::vector<std::vector<BrushParticle*>> fastBrushGrid;

void update_rainbow_fragments() {
    size_t count = rainbowFragments.size();
    for (size_t i = 0; i < count; ) {
        RainbowFragment& rf = rainbowFragments[i];

        rf.t += 0.016f;
        rf.vx *= 0.98f;
        rf.vy *= 0.98f;
        rf.x += rf.vx;
        rf.y += rf.vy;

        bool isDead = (rf.t > rf.life || rf.size < 1.5f);

        if (isDead) {
            rainbowFragments[i] = rainbowFragments.back();
            rainbowFragments.pop_back();
            count--;
        }
        else {
            i++;
        }
    }
}

void update_brush_particles(bool brushMode, bool& playerSunMode, float& playerSunTimer) {

    int gridCols = (SCREEN_WIDTH / (int)BRUSH_GRID_CELL_SIZE) + 1;
    int gridRows = (SCREEN_HEIGHT / (int)BRUSH_GRID_CELL_SIZE) + 1;
    if (fastBrushGrid.size() != gridCols * gridRows) fastBrushGrid.resize(gridCols * gridRows);
    for (auto& cell : fastBrushGrid) cell.clear();

    for (size_t k = 0; k < brushParticles.size(); ++k) {
        if (brushParticles[k].type == BRUSH_BLUE && !brushParticles[k].absorbed) {
            int cx = (int)(brushParticles[k].x / BRUSH_GRID_CELL_SIZE);
            int cy = (int)(brushParticles[k].y / BRUSH_GRID_CELL_SIZE);
            if (cx >= 0 && cx < gridCols && cy >= 0 && cy < gridRows) {
                fastBrushGrid[cx + cy * gridCols].push_back(&brushParticles[k]);
            }
        }
    }

    size_t count = brushParticles.size();
    size_t i = 0;

    while (i < count) {
        int type = brushParticles[i].type;

        if (type == BRUSH_DROP) {
            if (!brushParticles[i].absorbed) {
                brushParticles[i].vy += 0.15f;
                brushParticles[i].x += brushParticles[i].vx;
                brushParticles[i].y += brushParticles[i].vy;

                bool hitWater = false;
                if (brushParticles[i].y > SCREEN_HEIGHT * 0.1f) {
                    int bx = (int)(brushParticles[i].x / DENSITY_BUFFER_SCALE);
                    int by = (int)((brushParticles[i].y + 20) / DENSITY_BUFFER_SCALE);
                    if (bx >= 0 && bx < density_buffer_width && by >= 0 && by < density_buffer_height) {
                        if (density_buffer[by * density_buffer_width + bx] > 0.5f) hitWater = true;
                    }
                }
                if (brushParticles[i].y > SCREEN_HEIGHT - 10) hitWater = true;

                if (hitWater) {
                    brushParticles[i].absorbed = true;
                    brushParticles[i].dissolveFrame = 1;
                    if (rand() % 20 == 0) request_play(explosionSound);
                    if (brushParticles.size() < MAX_BRUSH_PARTICLES) {
                        BrushParticle boom;
                        float tx = brushParticles[i].x; float ty = brushParticles[i].y;
                        boom.x = tx; boom.y = ty;
                        boom.baseSize = 100.0f; boom.type = BRUSH_EXPLOSIVE;
                        boom.dissolveFrame = 4; boom.absorbed = false;
                        brushParticles.push_back(boom);
                    }
                    if (rand() % 3 == 0) spawnExplosionFragments(brushParticles[i].x, brushParticles[i].y);
                    if (rand() % 5 == 0) spawnRainbowFragments(brushParticles[i].x, brushParticles[i].y, 0, 0.8f);
                }
            }
        }
        else if (type == BRUSH_EXPLOSIVE) {
            if (!brushParticles[i].absorbed) {
                brushParticles[i].x += ((rand() % 10) - 5) * 0.2f;
                brushParticles[i].y += ((rand() % 10) - 5) * 0.2f;
                if (rand() % 4 == 0) {
                    RainbowFragment spark;
                    spark.x = brushParticles[i].x + ((rand() % 10) - 5);
                    spark.y = brushParticles[i].y + ((rand() % 10) - 5);
                    spark.vx = ((rand() % 10) - 5) * 0.3f;
                    spark.vy = -1.0f - (rand() % 10) * 0.2f;
                    spark.life = 0.5f; spark.size = 4.0f; spark.t = 0; spark.type = 0; spark.alpha0 = 0.8f;
                    if (rainbowFragments.size() < MAX_RAINBOW_FRAGMENTS) rainbowFragments.push_back(spark);
                }
                if (!brushMode && brushParticles[i].t > 2.0f) {
                    for (auto& p : particles) {
                        if (p.isPlayer) {
                            float dx = p.x - brushParticles[i].x; float dy = p.y - brushParticles[i].y;
                            if (dx * dx + dy * dy < (brushParticles[i].baseSize * 0.8f) * (brushParticles[i].baseSize * 0.8f)) {
                                brushParticles[i].absorbed = true; brushParticles[i].dissolveFrame = 1;
                                request_play(explosionSound);
                                playerSunMode = true; playerSunTimer = 10.0f;
                                spawnExplosionFragments(brushParticles[i].x, brushParticles[i].y);
                                break;
                            }
                        }
                    }
                }
            }
        }
        else {
            float springStiffness = 0.08f;
            float dx = brushParticles[i].baseX - brushParticles[i].x;
            float dy = brushParticles[i].baseY - brushParticles[i].y;
            brushParticles[i].vx += dx * springStiffness;
            brushParticles[i].vy += dy * springStiffness;
            brushParticles[i].x += brushParticles[i].vx;
            brushParticles[i].y += brushParticles[i].vy;
            const float BRUSH_VEL_DAMP = 0.82f;
            brushParticles[i].vx *= BRUSH_VEL_DAMP;
            brushParticles[i].vy *= BRUSH_VEL_DAMP;
        }

        brushParticles[i].t += 0.12f;
        brushParticles[i].impact *= 0.85f;
        if (brushParticles[i].dissolveFrame > 0) {
            brushParticles[i].dissolveFrame++;
            if (type == BRUSH_RAINBOW && brushParticles[i].dissolveFrame == 1) {
                spawnRainbowFragments(brushParticles[i].x, brushParticles[i].y, brushParticles[i].t, 44); request_play(rainbowSound);
            }
            if (type == BRUSH_BLUE && brushParticles[i].dissolveFrame == 1) request_play(blueSound);
        }
        else {
            if (type == BRUSH_BLUE) {
                if (brushParticles[i].impact > 15.0f) brushParticles[i].highImpactFrames++; else brushParticles[i].highImpactFrames = 0;
                if (brushParticles[i].highImpactFrames > 8) brushParticles[i].dissolveFrame = 1;
            }
        }

        if (brushParticles[i].dissolveFrame > 8) {
            brushParticles[i] = brushParticles.back();
            brushParticles.pop_back();
            count--;
        }
        else {
            i++;
        }
    }
    if (brushParticles.size() > MAX_BRUSH_PARTICLES) brushParticles.resize(MAX_BRUSH_PARTICLES);
}

void resolve_brush_collisions(bool brushMode, float avgPlayerVx, float avgPlayerVy, bool& playerRainbow, float& playerRainbowTimer, float& playerJumpTimer) {
    int brushGridCols = (SCREEN_WIDTH / (int)BRUSH_GRID_CELL_SIZE) + 1;
    int brushGridRows = (SCREEN_HEIGHT / (int)BRUSH_GRID_CELL_SIZE) + 1;

    if (fastBrushGrid.size() != brushGridCols * brushGridRows) fastBrushGrid.resize(brushGridCols * brushGridRows);
    for (auto& cell : fastBrushGrid) cell.clear();

    for (size_t i = 0; i < brushParticles.size(); ++i) {
        BrushParticle& bp = brushParticles[i];
        if (!bp.absorbed) {
            int cx = (int)(bp.x / BRUSH_GRID_CELL_SIZE);
            int cy = (int)(bp.y / BRUSH_GRID_CELL_SIZE);
            if (cx >= 0 && cx < brushGridCols && cy >= 0 && cy < brushGridRows) {
                fastBrushGrid[cx + cy * brushGridCols].push_back(&bp);
            }
        }
    }

    const float BRUSH_SURFACE_FACTOR = 0.55f;
    const float SURFACE_RADIUS_FACTOR = 0.6f;
    const float TANGENTIAL_FRICTION = 0.98f;

    float playerDirX = 0.0f, playerDirY = 0.0f;
    float playerSpeed = std::sqrt(avgPlayerVx * avgPlayerVx + avgPlayerVy * avgPlayerVy);
    bool playerMoving = (playerSpeed > 1e-3f);
    if (playerMoving) { playerDirX = avgPlayerVx / playerSpeed; playerDirY = avgPlayerVy / playerSpeed; }

    for (auto& p : particles) {
        int cx = (int)(p.x / BRUSH_GRID_CELL_SIZE);
        int cy = (int)(p.y / BRUSH_GRID_CELL_SIZE);

        for (int ny = cy - 1; ny <= cy + 1; ++ny) {
            for (int nx = cx - 1; nx <= cx + 1; ++nx) {
                if (nx >= 0 && nx < brushGridCols && ny >= 0 && ny < brushGridRows) {
                    for (BrushParticle* bp_ptr : fastBrushGrid[nx + ny * brushGridCols]) {
                        BrushParticle& bp = *bp_ptr;

                        if (bp.absorbed) continue;

                        if (!p.isPlayer) {
                            if (bp.type == BRUSH_BLUE) {
                                float baseR = bp.baseSize * BRUSH_SURFACE_FACTOR;
                                float surfaceR = baseR * SURFACE_RADIUS_FACTOR;
                                float dx = p.x - bp.x; float dy = p.y - bp.y;
                                float dist2 = dx * dx + dy * dy;
                                if (dist2 < surfaceR * surfaceR) {
                                    float dist = std::sqrt(dist2);
                                    if (dist < 1e-4f) { dx = 0.0f; dy = -1.0f; dist = 1.0f; }
                                    float nx_val = dx / dist; float ny_val = dy / dist;
                                    p.x = bp.x + nx_val * surfaceR; p.y = bp.y + ny_val * surfaceR;
                                    float vn = p.vx * nx_val + p.vy * ny_val;
                                    if (vn < 0.0f) { p.vx = (p.vx - vn * nx_val) * TANGENTIAL_FRICTION; p.vy = (p.vy - vn * ny_val) * TANGENTIAL_FRICTION; }
                                     bp.hasWater = true;
                                }
                            }
                        }
                        else if (p.isPlayer) {
                            if (bp.type == BRUSH_BLUE) {
                                float dx = bp.x - p.x; float dy = bp.y - p.y;
                                float dist2 = dx * dx + dy * dy;
                                float interactR = bp.baseSize * 0.85f;
                                if (dist2 < interactR * interactR) {
                                    float dist = std::sqrt(dist2);
                                    float dirX = (dist > 0.001f) ? dx / dist : 0.0f; float dirY = (dist > 0.001f) ? dy / dist : 1.0f;
                                    float influence = (1.0f - dist / interactR); influence *= influence;
                                    float pushX = dirX * 1.2f; float pushY = dirY * 1.2f;
                                    float flowX = 0.0f, flowY = 0.0f;
                                    if (playerMoving && (playerDirX * dirX + playerDirY * dirY > -0.5f)) {
                                        flowX = playerDirX * 0.8f; flowY = playerDirY * 0.8f;
                                    }
                                    bp.vx += (pushX + flowX) * influence * 0.6f;
                                    bp.vy += (pushY + flowY) * influence * 0.6f;
                                    bp.impact += influence * 1.5f;
                                    if (bp.impact > 10.0f) {
                                        static Uint32 lastTrig = 0;
                                        if (SDL_GetTicks() - lastTrig > 80) { request_play(blueSound); lastTrig = SDL_GetTicks(); }
                                    }
                                }
                            }

                            else if (bp.type == BRUSH_RAINBOW) {
                                float dx = p.x - bp.x; float dy = p.y - bp.y;
                        
                                if (dx * dx + dy * dy < (bp.baseSize * 0.7f) * (bp.baseSize * 0.7f)) {
                                    bp.absorbed = true; 
                                    bp.dissolveFrame = 1; 

                                    playerRainbow = true;
                                    playerRainbowTimer = 5.0f;
                                    playerJumpTimer = 0.5f;

                                    spawnRainbowFragments(bp.x, bp.y, bp.t, 0.4f);
                                    request_play(rainbowSound);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void apply_heat_from_fragments(const SpatialGrid& grid) {
    int skipCounter = 0;

    for (const auto& rf : rainbowFragments) {
        if (rf.type == 0 || rf.type == 3 || rf.type == 4) continue;
        skipCounter++;
        if (skipCounter % 2 != 0) continue;

        if (rf.x < 0 || rf.x >= SCREEN_WIDTH || rf.y < 0 || rf.y >= SCREEN_HEIGHT) continue;

        int cx = (int)(rf.x * grid.invCellSize);
        int cy = (int)(rf.y * grid.invCellSize);

        if (cx < 0 || cx >= grid.cols || cy < 0 || cy >= grid.rows) continue;

        for (int ny = cy - 1; ny <= cy + 1; ++ny) {
            if (ny < 0 || ny >= grid.rows) continue;
            int y_offset = ny * grid.cols;

            for (int nx = cx - 1; nx <= cx + 1; ++nx) {
                if (nx < 0 || nx >= grid.cols) continue;

                int cell_idx = nx + y_offset;

                int start = grid.cellStart[cell_idx];
                int count = grid.cellCount[cell_idx];
                int end = start + count;

                for (int i = start; i < end; ++i) {
                    Particle& p = particles[i];
                    if (!p.isPlayer) {
                        if (abs(p.x - rf.x) < 30.0f && abs(p.y - rf.y) < 30.0f) {
                            float dx = p.x - rf.x;
                            float dy = p.y - rf.y;
                            if (dx * dx + dy * dy < 900.0f) {
                                p.temperature = 3.0f;
                                p.vx += rf.vx * 0.15f;
                                p.vy += rf.vy * 0.15f;
                            }
                        }
                    }
                }
            }
        }
    }
}
