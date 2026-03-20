#include "physics.h"
#include "scene.h"
#include <cstdio>
#include <cmath>
#include <cassert>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s (line %d): %s\n", msg, __LINE__, #cond); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while(0)

void test_vec2() {
    Vec2 a(3, 4);
    CHECK(std::abs(a.length() - 5.0f) < 0.001f, "vec2 length");
    CHECK(std::abs(a.dot(Vec2(1, 0)) - 3.0f) < 0.001f, "vec2 dot");
    Vec2 n = a.normalized();
    CHECK(std::abs(n.length() - 1.0f) < 0.001f, "vec2 normalized");
    CHECK(a.is_finite(), "vec2 finite");
}

void test_gravity_single_ball() {
    PhysicsWorld world;
    world.config.gravity = 100.0f;
    world.config.substeps = 1;
    world.config.dt = 1.0f / 60.0f;
    world.config.damping = 1.0f; // no damping for this test

    Ball b;
    b.pos = {100, 100};
    b.vel = {0, 0};
    b.radius = 5;
    b.inv_mass = 1;
    world.balls.push_back(b);

    world.step();

    // Ball should have moved downward
    CHECK(world.balls[0].pos.y > 100.0f, "gravity moves ball down");
    CHECK(world.balls[0].vel.y > 0.0f, "gravity increases downward vel");
}

void test_ball_wall_collision() {
    PhysicsWorld world;
    world.config.gravity = 0.0f;
    world.config.substeps = 1;
    world.config.dt = 1.0f / 60.0f;
    world.config.restitution = 1.0f;
    world.config.slop = 0.0f;
    world.config.bounce_threshold = 0.0f;
    world.config.damping = 1.0f;

    // Floor at y=200
    world.walls.push_back({Vec2{0, 200}, Vec2{400, 200}});

    Ball b;
    b.pos = {100, 197}; // overlapping floor (ball bottom at 202)
    b.vel = {0, 100};   // moving down toward floor
    b.radius = 5;
    b.inv_mass = 1;
    world.balls.push_back(b);

    world.step();

    // Ball should have bounced (velocity reversed or at least upward)
    CHECK(world.balls[0].vel.y < 0.0f, "ball bounces off wall");
    CHECK(world.balls[0].pos.y < 200.0f, "ball stays above wall");
}

void test_ball_ball_collision() {
    PhysicsWorld world;
    world.config.gravity = 0.0f;
    world.config.substeps = 1;
    world.config.dt = 1.0f / 60.0f;
    world.config.restitution = 1.0f;
    world.config.slop = 0.0f;
    world.config.bounce_threshold = 0.0f;
    world.config.damping = 1.0f;

    Ball a, b;
    a.pos = {100, 100};
    a.vel = {50, 0};
    a.radius = 10;
    a.inv_mass = 1;

    b.pos = {118, 100}; // slightly overlapping
    b.vel = {-50, 0};
    b.radius = 10;
    b.inv_mass = 1;

    world.balls.push_back(a);
    world.balls.push_back(b);

    world.step();

    // Balls should have separated
    float dist = (world.balls[0].pos - world.balls[1].pos).length();
    CHECK(dist >= 19.0f, "balls separate after collision");
    // Velocities should have reversed (approximately)
    CHECK(world.balls[0].vel.x < 0.0f, "ball 0 reversed");
    CHECK(world.balls[1].vel.x > 0.0f, "ball 1 reversed");
}

void test_no_nan_after_many_steps() {
    PhysicsWorld world;
    SceneParams p;
    p.ball_count = 100;
    p.seed = 42;
    p.restitution = 0.5f;
    setup_scene(world, p);

    for (int i = 0; i < 500; ++i) {
        world.step();
    }

    SimMetrics m = world.compute_metrics();
    CHECK(!m.has_nan, "no NaN after 500 frames");
    CHECK(m.escaped_count == 0, "no escaped balls");
    CHECK(m.max_speed < 2500.0f, "speed bounded");
}

void test_scene_setup() {
    PhysicsWorld world;
    SceneParams p;
    p.ball_count = 1000;
    p.seed = 1;
    setup_scene(world, p);

    CHECK((int)world.balls.size() == 1000, "1000 balls created");
    CHECK(world.walls.size() > 5, "walls created");

    // Check no initial overlaps (balls shouldn't heavily overlap)
    int overlaps = 0;
    for (int i = 0; i < (int)world.balls.size(); ++i) {
        for (int j = i + 1; j < (int)world.balls.size(); ++j) {
            float d = (world.balls[i].pos - world.balls[j].pos).length();
            if (d < world.balls[i].radius + world.balls[j].radius - 1.0f) {
                overlaps++;
            }
        }
    }
    CHECK(overlaps < 50, "minimal initial overlaps");
}

void test_metrics() {
    PhysicsWorld world;
    Ball b;
    b.pos = {100, 100};
    b.vel = {10, 0};
    b.radius = 5;
    b.inv_mass = 1;
    world.balls.push_back(b);

    SimMetrics m = world.compute_metrics();
    CHECK(m.ball_count == 1, "metrics ball count");
    CHECK(m.max_speed > 9.0f, "metrics max speed");
    CHECK(!m.has_nan, "metrics no NaN");
}

int main() {
    printf("Running physics unit tests...\n\n");

    test_vec2();
    test_gravity_single_ball();
    test_ball_wall_collision();
    test_ball_ball_collision();
    test_no_nan_after_many_steps();
    test_scene_setup();
    test_metrics();

    printf("\n%d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
