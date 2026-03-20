#pragma once
#include "vec2.h"
#include "spatial_hash.h"
#include <vector>
#include <cstdint>
#include <string>

struct Ball {
    Vec2 pos;
    Vec2 vel;
    float radius = 5.0f;
    float inv_mass = 1.0f; // 1/mass; 0 = infinite mass (static)
    uint32_t color = 0xFFFFFFFF;
    bool sleeping = false;
    float sleep_timer = 0.0f;
};

struct Wall {
    Vec2 a, b; // segment endpoints
};

struct SimConfig {
    float gravity = 500.0f;       // pixels/s^2 downward
    float restitution = 0.3f;     // coefficient of restitution [0,1]
    float friction = 0.3f;        // tangential friction coefficient
    float damping = 0.999f;       // velocity damping per substep
    float dt = 1.0f / 60.0f;      // frame timestep
    int substeps = 8;             // physics substeps per frame
    float slop = 0.5f;            // penetration slop before correction
    float correction_frac = 0.5f; // positional correction fraction
    float max_correction = 2.0f;  // max positional correction per step
    float bounce_threshold = 30.0f; // min relative vel for bounce
    float sleep_threshold = 8.0f;  // speed below which sleep timer ticks
    float sleep_time = 0.4f;       // seconds of low speed before sleeping
    float wake_threshold = 15.0f;  // impact speed above which sleeping ball wakes
    float max_speed = 2000.0f;    // speed clamp
    int solver_iterations = 4;    // contact solver iterations
};

struct SimMetrics {
    int frame = 0;
    int ball_count = 0;
    int sleeping_count = 0;
    float max_speed = 0.0f;
    float max_penetration = 0.0f;
    float total_kinetic_energy = 0.0f;
    int escaped_count = 0;
    bool has_nan = false;
    float min_y = 0.0f; // top of pile (lowest y = highest on screen)
    float max_y = 0.0f; // bottom of pile
    float pile_height = 0.0f;
};

class PhysicsWorld {
public:
    SimConfig config;
    std::vector<Ball> balls;
    std::vector<Wall> walls;

    void step(); // advance one frame (config.substeps sub-steps)
    SimMetrics compute_metrics() const;

    // bounds for escape detection
    float bound_left = -100, bound_right = 1400;
    float bound_top = -500, bound_bottom = 1200;

private:
    SpatialHash grid_;
    std::vector<std::pair<int,int>> contact_pairs_;
    float max_radius_ = 0.0f;

    void substep(float sub_dt);
    void integrate(float sub_dt);
    void build_grid();
    void build_contact_pairs();
    void solve_contacts(float sub_dt);
    void solve_ball_ball(int i, int j, float sub_dt);
    void solve_ball_wall(int i, const Wall& w, float sub_dt);
    void clamp_speeds();
    void update_sleep(float sub_dt);
};
