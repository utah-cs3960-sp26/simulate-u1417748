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
