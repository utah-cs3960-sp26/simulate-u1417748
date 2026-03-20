#include "physics.h"
#include "scene.h"
#include "renderer.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <string>

static void print_usage() {
    printf("Usage: simulate [options]\n");
    printf("  --balls N         Number of balls (default: 1000)\n");
    printf("  --seed N          RNG seed (default: 1)\n");
    printf("  --restitution F   Coefficient of restitution 0..1 (default: 0.3)\n");
    printf("  --scene NAME      Scene name (default: main)\n");
    printf("  --substeps N      Physics substeps per frame (default: 8)\n");
    printf("  --help            Show this help\n");
    printf("\nControls:\n");
    printf("  Space     Pause/Resume\n");
    printf("  R         Reset scene\n");
    printf("  S         Single step (when paused)\n");
    printf("  D         Toggle debug overlay\n");
    printf("  Up/Down   Adjust restitution\n");
    printf("  Q/Escape  Quit\n");
}

int main(int argc, char* argv[]) {
    SceneParams scene_params;
    int substeps = 8;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--balls") == 0 && i + 1 < argc)
            scene_params.ball_count = atoi(argv[++i]);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            scene_params.seed = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--restitution") == 0 && i + 1 < argc)
            scene_params.restitution = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc)
            scene_params.name = argv[++i];
        else if (strcmp(argv[i], "--substeps") == 0 && i + 1 < argc)
            substeps = atoi(argv[++i]);
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
    }

    const int W = 1200, H = 800;

    Renderer ren;
    if (!ren.init(W, H)) return 1;

    PhysicsWorld world;
    world.config.substeps = substeps;
    setup_scene(world, scene_params);

    bool running = true;
    bool paused = false;
    bool show_debug = true;
    bool step_once = false;

    Uint64 last_time = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    float fps = 60.0f;
    int frame = 0;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                switch (ev.key.key) {
                    case SDLK_ESCAPE:
                    case SDLK_Q:
                        running = false;
                        break;
                    case SDLK_SPACE:
                        paused = !paused;
                        break;
                    case SDLK_R:
                        setup_scene(world, scene_params);
                        frame = 0;
                        break;
                    case SDLK_S:
                        if (paused) step_once = true;
                        break;
                    case SDLK_D:
                        show_debug = !show_debug;
                        break;
                    case SDLK_UP:
                        world.config.restitution = std::min(1.0f, world.config.restitution + 0.05f);
                        printf("Restitution: %.2f\n", world.config.restitution);
                        break;
                    case SDLK_DOWN:
                        world.config.restitution = std::max(0.0f, world.config.restitution - 0.05f);
                        printf("Restitution: %.2f\n", world.config.restitution);
                        break;
                    default: break;
                }
            }
        }

        if (!paused || step_once) {
            world.step();
            frame++;
            step_once = false;
        }

        // Compute metrics (sampled, not every frame for perf)
        SimMetrics metrics = {};
        if (frame % 30 == 0 || show_debug) {
            metrics = world.compute_metrics();
            metrics.frame = frame;
        }

        // FPS
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last_time) / (float)freq;
        last_time = now;
        fps = fps * 0.95f + (1.0f / std::max(dt, 0.0001f)) * 0.05f;

        ren.draw(world, metrics, paused, show_debug, fps);

        // Print periodic status
        if (frame % 300 == 0 && frame > 0) {
            metrics = world.compute_metrics();
            printf("[Frame %d] balls=%d sleeping=%d maxSpeed=%.1f maxPen=%.2f KE=%.1f escaped=%d\n",
                   frame, metrics.ball_count, metrics.sleeping_count,
                   metrics.max_speed, metrics.max_penetration,
                   metrics.total_kinetic_energy, metrics.escaped_count);
        }

        // Cap to ~60 fps
        SDL_Delay(16);
    }

    ren.shutdown();
    return 0;
}
