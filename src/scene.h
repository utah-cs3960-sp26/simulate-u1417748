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
