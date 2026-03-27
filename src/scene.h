#pragma once
#include "physics.h"
#include <cstdint>
#include <string>

struct SceneParams {
    std::string name = "main";
    int ball_count = 1000;
    uint32_t seed = 1;
    float restitution = 0.3f;
};

void setup_scene(PhysicsWorld& world, const SceneParams& params);

// Load balls from a CSV file; returns true on success.
// Walls are set up from the default scene layout.
bool load_scene_csv(PhysicsWorld& world, const std::string& path, float restitution);

// Save current ball positions to a CSV file; returns true on success.
bool save_scene_csv(const PhysicsWorld& world, const std::string& path);

// Save custom positions/velocities with current ball colors to a CSV file.
// Writes positions[i], velocities[i] with world.balls[i].color.
bool save_scene_csv_with_positions(const PhysicsWorld& world,
                                   const std::vector<Vec2>& positions,
                                   const std::vector<Vec2>& velocities,
                                   const std::vector<float>& radii,
                                   const std::string& path);
