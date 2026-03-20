#include "physics.h"
#include "scene.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <chrono>

static void print_usage() {
    printf("Usage: simulate_headless [options]\n");
    printf("  --balls N         Number of balls (default: 1000)\n");
    printf("  --seed N          RNG seed (default: 1)\n");
    printf("  --restitution F   Coefficient of restitution 0..1 (default: 0.3)\n");
    printf("  --scene NAME      Scene name (default: main)\n");
    printf("  --frames N        Number of frames to simulate (default: 3000)\n");
    printf("  --substeps N      Physics substeps per frame (default: 8)\n");
    printf("  --metrics         Print metrics at end\n");
    printf("  --verbose         Print periodic metrics\n");
    printf("  --help            Show this help\n");
}

int main(int argc, char* argv[]) {
    SceneParams scene_params;
    int frames = 3000;
    int substeps = 8;
    bool metrics = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--balls") == 0 && i + 1 < argc)
            scene_params.ball_count = atoi(argv[++i]);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            scene_params.seed = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--restitution") == 0 && i + 1 < argc)
            scene_params.restitution = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc)
            scene_params.name = argv[++i];
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
            frames = atoi(argv[++i]);
        else if (strcmp(argv[i], "--substeps") == 0 && i + 1 < argc)
            substeps = atoi(argv[++i]);
        else if (strcmp(argv[i], "--metrics") == 0)
            metrics = true;
        else if (strcmp(argv[i], "--verbose") == 0)
            verbose = true;
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
    }

    PhysicsWorld world;
    world.config.substeps = substeps;
    setup_scene(world, scene_params);

    printf("Headless simulation: balls=%d seed=%u restitution=%.2f frames=%d substeps=%d\n",
           scene_params.ball_count, scene_params.seed,
           scene_params.restitution, frames, substeps);

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int f = 0; f < frames; ++f) {
        world.step();

        if (verbose && (f % 300 == 0)) {
            SimMetrics m = world.compute_metrics();
            m.frame = f;
            printf("  [Frame %d] sleeping=%d maxSpeed=%.1f maxPen=%.2f KE=%.1f escaped=%d\n",
                   f, m.sleeping_count, m.max_speed, m.max_penetration,
                   m.total_kinetic_energy, m.escaped_count);
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    SimMetrics m = world.compute_metrics();
    m.frame = frames;

    printf("Completed in %.2f seconds (%.1f fps)\n", elapsed, frames / elapsed);

    if (metrics) {
        printf("\n=== FINAL METRICS ===\n");
        printf("frame:          %d\n", m.frame);
        printf("balls:          %d\n", m.ball_count);
        printf("sleeping:       %d\n", m.sleeping_count);
        printf("max_speed:      %.2f\n", m.max_speed);
        printf("max_penetration:%.2f\n", m.max_penetration);
        printf("kinetic_energy: %.2f\n", m.total_kinetic_energy);
        printf("escaped:        %d\n", m.escaped_count);
        printf("has_nan:        %s\n", m.has_nan ? "YES" : "no");
        printf("pile_min_y:     %.1f\n", m.min_y);
        printf("pile_max_y:     %.1f\n", m.max_y);
        printf("pile_height:    %.1f\n", m.pile_height);
        printf("=====================\n");
    }

    // Validation checks (non-zero exit on failure)
    bool pass = true;
    if (m.has_nan) {
        fprintf(stderr, "FAIL: NaN detected in simulation state\n");
        pass = false;
    }
    if (m.escaped_count > 0) {
        fprintf(stderr, "FAIL: %d balls escaped bounds\n", m.escaped_count);
        pass = false;
    }
    if (m.max_penetration > 5.0f) {
        fprintf(stderr, "FAIL: max penetration %.2f > 5.0\n", m.max_penetration);
        pass = false;
    }
    if (m.max_speed > 1500.0f) {
        fprintf(stderr, "FAIL: max speed %.2f > 1500\n", m.max_speed);
        pass = false;
    }

    if (pass) {
        printf("PASS: all validation checks passed\n");
    }

    return pass ? 0 : 1;
}
