#include "scene.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>

// Simple xorshift32 for deterministic RNG
struct Rng {
    uint32_t state;
    Rng(uint32_t seed) : state(seed ? seed : 1) {}
    uint32_t next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float uniform() { return (float)(next() & 0xFFFFFF) / (float)0xFFFFFF; }
    float range(float lo, float hi) { return lo + uniform() * (hi - lo); }
};

static uint32_t make_color(Rng& rng) {
    uint8_t r = 100 + (uint8_t)(rng.uniform() * 155);
    uint8_t g = 100 + (uint8_t)(rng.uniform() * 155);
    uint8_t b = 100 + (uint8_t)(rng.uniform() * 155);
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void setup_scene(PhysicsWorld& world, const SceneParams& params) {
    world.balls.clear();
    world.walls.clear();
    world.config.restitution = params.restitution;

    // Window: 1200x800
    // Hopper/funnel shape: wide top → pegs → funnel → settling basin
    //
    //   |                              |   <- wide top (spawn area)
    //   |   ___  ___  ___  ___  ___    |   <- peg rows
    //   |     ___  ___  ___  ___       |
    //    \                            /    <- funnel slopes
    //     \                          /
    //      |                        |      <- narrow settling basin
    //      |________________________|

    const float W = 1200.0f;
    const float H = 800.0f;

    // Container dimensions
    float left = 80.0f;
    float right = W - 80.0f;  // 1120
    float top_y = 30.0f;
    float funnel_start_y = 350.0f;   // where funnel slopes begin
    float funnel_end_y = 500.0f;     // where funnel ends
    float basin_left = 300.0f;
    float basin_right = 900.0f;
    float bottom_y = H - 40.0f;     // 760

    // Left wall (top portion)
    world.walls.push_back({Vec2{left, top_y}, Vec2{left, funnel_start_y}});
    // Left funnel slope
    world.walls.push_back({Vec2{left, funnel_start_y}, Vec2{basin_left, funnel_end_y}});
    // Left basin wall
    world.walls.push_back({Vec2{basin_left, funnel_end_y}, Vec2{basin_left, bottom_y - 30}});
    // Bottom-left chamfer
    world.walls.push_back({Vec2{basin_left, bottom_y - 30}, Vec2{basin_left + 30, bottom_y}});

    // Right wall (top portion)
    world.walls.push_back({Vec2{right, top_y}, Vec2{right, funnel_start_y}});
    // Right funnel slope
    world.walls.push_back({Vec2{right, funnel_start_y}, Vec2{basin_right, funnel_end_y}});
    // Right basin wall
    world.walls.push_back({Vec2{basin_right, funnel_end_y}, Vec2{basin_right, bottom_y - 30}});
    // Bottom-right chamfer
    world.walls.push_back({Vec2{basin_right, bottom_y - 30}, Vec2{basin_right - 30, bottom_y}});

    // Floor
    world.walls.push_back({Vec2{basin_left + 30, bottom_y}, Vec2{basin_right - 30, bottom_y}});

    // Peg rows in upper section for visual interest
    // Row 1 at y=150: horizontal pegs
    for (float px = left + 80; px < right - 60; px += 110) {
        world.walls.push_back({Vec2{px, 150}, Vec2{px + 35, 150}});
    }
    // Row 2 at y=220: offset pegs
    for (float px = left + 135; px < right - 60; px += 110) {
        world.walls.push_back({Vec2{px, 220}, Vec2{px + 35, 220}});
    }
    // Row 3 at y=290: angled deflectors
    for (float px = left + 80; px < right - 60; px += 130) {
        world.walls.push_back({Vec2{px, 285}, Vec2{px + 40, 295}});
    }

    // A few angled internal baffles in the funnel transition to slow flow
    world.walls.push_back({Vec2{400, 400}, Vec2{500, 420}});
    world.walls.push_back({Vec2{700, 400}, Vec2{800, 420}});
    world.walls.push_back({Vec2{550, 450}, Vec2{650, 470}});

    // Spawn balls in the upper wide region
    Rng rng(params.seed);
    float ball_r = 5.0f;
    float spawn_left = left + ball_r + 5;
    float spawn_right = right - ball_r - 5;
    float spawn_top = top_y + ball_r + 5;
    float spawn_bot = funnel_start_y - ball_r - 20;

    float spacing = ball_r * 2.3f;
    int cols = (int)((spawn_right - spawn_left) / spacing);
    int rows_avail = (int)((spawn_bot - spawn_top) / spacing);

    int count = 0;
    for (int row = 0; row < rows_avail && count < params.ball_count; ++row) {
        for (int col = 0; col < cols && count < params.ball_count; ++col) {
            Ball b;
            b.radius = ball_r;
            b.pos.x = spawn_left + col * spacing + rng.range(-0.5f, 0.5f);
            b.pos.y = spawn_top + row * spacing + rng.range(-0.5f, 0.5f);
            b.vel = {rng.range(-40, 40), rng.range(-10, 60)};
            b.inv_mass = 1.0f;
            b.color = make_color(rng);
            world.balls.push_back(b);
            count++;
        }
    }

    // Set escape bounds
    world.bound_left = -200;
    world.bound_right = W + 200;
    world.bound_top = -300;
    world.bound_bottom = H + 400;
}

static void setup_walls_and_bounds(PhysicsWorld& world) {
    const float W = 1200.0f;
    const float H = 800.0f;

    float left = 80.0f;
    float right = W - 80.0f;
    float top_y = 30.0f;
    float funnel_start_y = 350.0f;
    float funnel_end_y = 500.0f;
    float basin_left = 300.0f;
    float basin_right = 900.0f;
    float bottom_y = H - 40.0f;

    world.walls.push_back({Vec2{left, top_y}, Vec2{left, funnel_start_y}});
    world.walls.push_back({Vec2{left, funnel_start_y}, Vec2{basin_left, funnel_end_y}});
    world.walls.push_back({Vec2{basin_left, funnel_end_y}, Vec2{basin_left, bottom_y - 30}});
    world.walls.push_back({Vec2{basin_left, bottom_y - 30}, Vec2{basin_left + 30, bottom_y}});

    world.walls.push_back({Vec2{right, top_y}, Vec2{right, funnel_start_y}});
    world.walls.push_back({Vec2{right, funnel_start_y}, Vec2{basin_right, funnel_end_y}});
    world.walls.push_back({Vec2{basin_right, funnel_end_y}, Vec2{basin_right, bottom_y - 30}});
    world.walls.push_back({Vec2{basin_right, bottom_y - 30}, Vec2{basin_right - 30, bottom_y}});

    world.walls.push_back({Vec2{basin_left + 30, bottom_y}, Vec2{basin_right - 30, bottom_y}});

    for (float px = left + 80; px < right - 60; px += 110) {
        world.walls.push_back({Vec2{px, 150}, Vec2{px + 35, 150}});
    }
    for (float px = left + 135; px < right - 60; px += 110) {
        world.walls.push_back({Vec2{px, 220}, Vec2{px + 35, 220}});
    }
    for (float px = left + 80; px < right - 60; px += 130) {
        world.walls.push_back({Vec2{px, 285}, Vec2{px + 40, 295}});
    }

    world.walls.push_back({Vec2{400, 400}, Vec2{500, 420}});
    world.walls.push_back({Vec2{700, 400}, Vec2{800, 420}});
    world.walls.push_back({Vec2{550, 450}, Vec2{650, 470}});

    world.bound_left = -200;
    world.bound_right = W + 200;
    world.bound_top = -300;
    world.bound_bottom = H + 400;
}

bool load_scene_csv(PhysicsWorld& world, const std::string& path, float restitution) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    world.balls.clear();
    world.walls.clear();
    world.config.restitution = restitution;

    setup_walls_and_bounds(world);

    std::string line;
    // Read header to detect format
    if (!std::getline(file, line)) return true;
    bool has_velocity = (line.find("vx") != std::string::npos);

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string token;

        Ball b;
        b.vel = {0, 0};
        b.inv_mass = 1.0f;

        if (!std::getline(ss, token, ',')) continue;
        b.pos.x = std::stof(token);

        if (!std::getline(ss, token, ',')) continue;
        b.pos.y = std::stof(token);

        if (has_velocity) {
            if (!std::getline(ss, token, ',')) continue;
            b.vel.x = std::stof(token);

            if (!std::getline(ss, token, ',')) continue;
            b.vel.y = std::stof(token);
        }

        if (!std::getline(ss, token, ',')) continue;
        b.radius = std::stof(token);

        if (!std::getline(ss, token, ',')) continue;
        uint32_t rgb = (uint32_t)std::stoul(token, nullptr, 16);
        b.color = 0xFF000000u | rgb;

        world.balls.push_back(b);
    }
    return true;
}

bool save_scene_csv(const PhysicsWorld& world, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << std::setprecision(std::numeric_limits<float>::max_digits10);
    file << "x,y,vx,vy,radius,color\n";
    for (const auto& b : world.balls) {
        uint32_t rgb = b.color & 0x00FFFFFFu;
        file << b.pos.x << "," << b.pos.y << ","
             << b.vel.x << "," << b.vel.y << ","
             << b.radius << ","
             << std::uppercase << std::hex << std::setfill('0') << std::setw(6) << rgb
             << std::dec << "\n";
    }
    return true;
}

bool save_scene_csv_with_positions(const PhysicsWorld& world,
                                   const std::vector<Vec2>& positions,
                                   const std::vector<Vec2>& velocities,
                                   const std::vector<float>& radii,
                                   const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << std::setprecision(std::numeric_limits<float>::max_digits10);
    file << "x,y,vx,vy,radius,color\n";
    size_t n = std::min({positions.size(), velocities.size(), world.balls.size()});
    for (size_t i = 0; i < n; ++i) {
        uint32_t rgb = world.balls[i].color & 0x00FFFFFFu;
        file << positions[i].x << "," << positions[i].y << ","
             << velocities[i].x << "," << velocities[i].y << ","
             << radii[i] << ","
             << std::uppercase << std::hex << std::setfill('0') << std::setw(6) << rgb
             << std::dec << "\n";
    }
    return true;
}
