#pragma once
#include "GameConfig.h"

void calculate_forces_for_keys(const std::vector<int>& cell_indices, const SpatialGrid& grid, std::vector<Vector2D>& local_forces);
void calculate_player_cohesion_forces(std::vector<Vector2D>& forces);
void calculate_mouse_interaction_forces(int mx, int my, bool leftDown, std::vector<Vector2D>& forces, bool playerSunMode);
void apply_forces_to_particles(std::vector<Vector2D>& forces);

void update_rainbow_fragments();

void update_brush_particles(bool brushMode, bool& playerSunMode, float& playerSunTimer);

void resolve_brush_collisions(bool brushMode, float avgPlayerVx, float avgPlayerVy, bool& playerRainbow, float& playerRainbowTimer, float& playerJumpTimer);

void apply_heat_from_fragments(const SpatialGrid& grid);
