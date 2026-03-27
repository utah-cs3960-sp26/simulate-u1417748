#include "physics.h"
#include "scene.h"
#include "renderer.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>

static void print_usage() {
    printf("Usage: simulate [options]\n");
    printf("  --balls N         Number of balls (default: 1000)\n");
    printf("  --seed N          RNG seed (default: 1)\n");
    printf("  --restitution F   Coefficient of restitution 0..1 (default: 0.3)\n");
    printf("  --scene NAME      Scene name (default: main)\n");
    printf("  --substeps N      Physics substeps per frame (default: 8)\n");
    printf("  --headless        Run headless (no window)\n");
    printf("  --frames N        Frames to simulate (headless, default: 3000)\n");
    printf("  --metrics         Print final metrics (headless)\n");
    printf("  --verbose         Print periodic metrics (headless)\n");
    printf("  --load-csv PATH   Load initial state from CSV file\n");
    printf("  --save-csv PATH   Save final state to CSV file\n");
    printf("  --help            Show this help\n");
    printf("\nControls:\n");
    printf("  Space     Pause/Resume\n");
    printf("  R         Reset scene\n");
    printf("  S         Single step (when paused)\n");
    printf("  D         Toggle debug overlay\n");
    printf("  E         End simulation and save CSV\n");
    printf("  Up/Down   Adjust restitution\n");
    printf("  Q/Escape  Quit\n");
}

static int run_headless(const SceneParams& scene_params, int substeps, int frames,
                        bool metrics, bool verbose,
                        const std::string& load_csv, const std::string& save_csv);

enum class AppState { START_SCREEN, SIMULATING, END_SCREEN };

// Callback data for SDL file dialog
struct FileDialogResult {
    std::string path;
    bool completed = false;
};

static void SDLCALL file_dialog_callback(void* userdata, const char* const* filelist, int /*filter*/) {
    auto* result = static_cast<FileDialogResult*>(userdata);
    if (filelist && filelist[0]) {
        result->path = filelist[0];
    }
    result->completed = true;
}

static std::string get_default_save_path() {
    const char* home = SDL_GetPrefPath("PhysicsSim", "simulate");
    std::string path;
    if (home) {
        path = std::string(home) + "results.csv";
    } else {
        path = "results.csv";
    }
    return path;
}

int main(int argc, char* argv[]) {
    SceneParams scene_params;
    int substeps = 8;
    bool headless = false;
    int frames = 3000;
    bool metrics_flag = false;
    bool verbose_flag = false;
    std::string load_csv;
    std::string save_csv;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--balls") == 0 && i + 1 < argc)
            scene_params.ball_count = std::max(1, atoi(argv[++i]));
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc)
            scene_params.seed = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--restitution") == 0 && i + 1 < argc)
            scene_params.restitution = std::clamp((float)atof(argv[++i]), 0.0f, 1.0f);
        else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc)
            scene_params.name = argv[++i];
        else if (strcmp(argv[i], "--substeps") == 0 && i + 1 < argc)
            substeps = std::max(1, atoi(argv[++i]));
        else if (strcmp(argv[i], "--headless") == 0)
            headless = true;
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
            frames = std::max(1, atoi(argv[++i]));
        else if (strcmp(argv[i], "--metrics") == 0)
            metrics_flag = true;
        else if (strcmp(argv[i], "--verbose") == 0)
            verbose_flag = true;
        else if (strcmp(argv[i], "--load-csv") == 0 && i + 1 < argc)
            load_csv = argv[++i];
        else if (strcmp(argv[i], "--save-csv") == 0 && i + 1 < argc)
            save_csv = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
    }

    if (headless) {
        return run_headless(scene_params, substeps, frames, metrics_flag, verbose_flag,
                            load_csv, save_csv);
    }

    const int W = 1200, H = 800;

    Renderer ren;
    if (!ren.init(W, H)) return 1;

    PhysicsWorld world;
    world.config.substeps = substeps;

    AppState state = AppState::START_SCREEN;

    // If a CSV was given on command line, skip the start screen
    if (!load_csv.empty()) {
        setup_scene(world, scene_params);
        if (!load_scene_csv(world, load_csv, scene_params.restitution)) {
            fprintf(stderr, "Failed to load CSV: %s\n", load_csv.c_str());
            ren.shutdown();
            return 1;
        }
        state = AppState::SIMULATING;
    }

    // Start screen buttons
    float btn_w = 350, btn_h = 70;
    float btn_x = (W - btn_w) / 2.0f;
    Button btn_start = {{btn_x, 400, btn_w, btn_h}, "START SIMULATION",
                        0xC0225522, 0xFF33AA33};
    Button btn_load  = {{btn_x, 500, btn_w, btn_h}, "LOAD FROM CSV",
                        0xC0224455, 0xFF3366AA};

    // End screen buttons
    Button btn_show    = {{btn_x, 370, btn_w, btn_h}, "SHOW IN FINDER",
                          0xC0444422, 0xFF88AA33};
    Button btn_restart = {{btn_x, 470, btn_w, btn_h}, "NEW SIMULATION",
                          0xC0225522, 0xFF33AA33};
    Button btn_quit    = {{btn_x, 570, btn_w, btn_h}, "QUIT",
                          0xC0552222, 0xFFAA3333};

    std::string saved_path;
    FileDialogResult dialog_result;

    bool running = true;
    bool paused = false;
    bool show_debug = true;
    bool step_once = false;

    Uint64 last_time = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    float fps = 60.0f;
    int frame = 0;

    while (running) {
        float mx, my;
        SDL_GetMouseState(&mx, &my);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }

            if (state == AppState::START_SCREEN) {
                // Check for pending file dialog result
                if (dialog_result.completed) {
                    dialog_result.completed = false;
                    if (!dialog_result.path.empty()) {
                        setup_scene(world, scene_params);
                        if (load_scene_csv(world, dialog_result.path, scene_params.restitution)) {
                            load_csv = dialog_result.path;
                            printf("Loaded scene from: %s (%d balls)\n",
                                   load_csv.c_str(), (int)world.balls.size());
                            state = AppState::SIMULATING;
                            frame = 0;
                        } else {
                            fprintf(stderr, "Failed to load CSV: %s\n", dialog_result.path.c_str());
                        }
                    }
                    dialog_result.path.clear();
                }

                if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    if (btn_start.contains(mx, my)) {
                        setup_scene(world, scene_params);
                        state = AppState::SIMULATING;
                        frame = 0;
                        load_csv.clear();
                    } else if (btn_load.contains(mx, my)) {
                        SDL_DialogFileFilter filters[] = {{"CSV Files", "csv"}, {"All Files", "*"}};
                        dialog_result = {};
                        SDL_ShowOpenFileDialog(file_dialog_callback, &dialog_result,
                                              ren.window, filters, 2, nullptr, false);
                    }
                }
                if (ev.type == SDL_EVENT_KEY_DOWN) {
                    if (ev.key.key == SDLK_ESCAPE || ev.key.key == SDLK_Q)
                        running = false;
                }

            } else if (state == AppState::SIMULATING) {
                if (ev.type == SDL_EVENT_KEY_DOWN) {
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
                            if (!load_csv.empty())
                                load_scene_csv(world, load_csv, scene_params.restitution);
                            frame = 0;
                            break;
                        case SDLK_S:
                            if (paused) step_once = true;
                            break;
                        case SDLK_D:
                            show_debug = !show_debug;
                            break;
                        case SDLK_E: {
                            // End simulation → save and show end screen
                            saved_path = save_csv.empty() ? get_default_save_path() : save_csv;
                            if (save_scene_csv(world, saved_path)) {
                                printf("Saved results to: %s\n", saved_path.c_str());
                            } else {
                                fprintf(stderr, "Failed to save CSV: %s\n", saved_path.c_str());
                                saved_path = "(SAVE FAILED)";
                            }
                            state = AppState::END_SCREEN;
                            break;
                        }
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

            } else if (state == AppState::END_SCREEN) {
                if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    if (btn_show.contains(mx, my)) {
                        std::string cmd = "open -R \"" + saved_path + "\"";
                        system(cmd.c_str());
                    } else if (btn_restart.contains(mx, my)) {
                        state = AppState::START_SCREEN;
                        load_csv.clear();
                        paused = false;
                        frame = 0;
                    } else if (btn_quit.contains(mx, my)) {
                        running = false;
                    }
                }
                if (ev.type == SDL_EVENT_KEY_DOWN) {
                    if (ev.key.key == SDLK_ESCAPE || ev.key.key == SDLK_Q)
                        running = false;
                    else if (ev.key.key == SDLK_R) {
                        state = AppState::START_SCREEN;
                        load_csv.clear();
                        paused = false;
                        frame = 0;
                    }
                }
            }
        }

        if (state == AppState::START_SCREEN) {
            btn_start.hovered = btn_start.contains(mx, my);
            btn_load.hovered = btn_load.contains(mx, my);
            ren.draw_start_screen(btn_start, btn_load);

        } else if (state == AppState::SIMULATING) {
            if (!paused || step_once) {
                world.step();
                frame++;
                step_once = false;
            }

            SimMetrics metrics = {};
            if (frame % 30 == 0 || show_debug) {
                metrics = world.compute_metrics();
                metrics.frame = frame;
            }

            Uint64 now = SDL_GetPerformanceCounter();
            float dt = (float)(now - last_time) / (float)freq;
            last_time = now;
            fps = fps * 0.95f + (1.0f / std::max(dt, 0.0001f)) * 0.05f;

            ren.draw(world, metrics, paused, show_debug, fps);

            if (frame % 300 == 0 && frame > 0) {
                metrics = world.compute_metrics();
                printf("[Frame %d] balls=%d sleeping=%d maxSpeed=%.1f maxPen=%.2f KE=%.1f escaped=%d\n",
                       frame, metrics.ball_count, metrics.sleeping_count,
                       metrics.max_speed, metrics.max_penetration,
                       metrics.total_kinetic_energy, metrics.escaped_count);
            }

        } else if (state == AppState::END_SCREEN) {
            btn_show.hovered = btn_show.contains(mx, my);
            btn_restart.hovered = btn_restart.contains(mx, my);
            btn_quit.hovered = btn_quit.contains(mx, my);
            ren.draw_end_screen(saved_path, btn_show, btn_restart, btn_quit);
        }

        SDL_Delay(16);
    }

    // Auto-save on quit if we were simulating and didn't already save
    if (state == AppState::SIMULATING && !save_csv.empty()) {
        if (!save_scene_csv(world, save_csv)) {
            fprintf(stderr, "Failed to save CSV: %s\n", save_csv.c_str());
        }
    }

    ren.shutdown();
    return 0;
}

static int run_headless(const SceneParams& scene_params, int substeps, int frames,
                        bool metrics, bool verbose,
                        const std::string& load_csv, const std::string& save_csv) {
    PhysicsWorld world;
    world.config.substeps = substeps;
    setup_scene(world, scene_params);
    if (!load_csv.empty()) {
        if (!load_scene_csv(world, load_csv, scene_params.restitution)) {
            fprintf(stderr, "Failed to load CSV: %s\n", load_csv.c_str());
            return 1;
        }
    }

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

    bool pass = true;
    if (m.has_nan) { fprintf(stderr, "FAIL: NaN detected\n"); pass = false; }
    if (m.escaped_count > 0) { fprintf(stderr, "FAIL: %d balls escaped\n", m.escaped_count); pass = false; }
    if (m.max_penetration > 5.0f) { fprintf(stderr, "FAIL: max penetration %.2f > 5.0\n", m.max_penetration); pass = false; }
    if (m.max_speed > 1500.0f) { fprintf(stderr, "FAIL: max speed %.2f > 1500\n", m.max_speed); pass = false; }
    if (!save_csv.empty()) {
        if (!save_scene_csv(world, save_csv)) {
            fprintf(stderr, "Failed to save CSV: %s\n", save_csv.c_str());
        }
    }

    if (pass) printf("PASS: all validation checks passed\n");
    return pass ? 0 : 1;
}
