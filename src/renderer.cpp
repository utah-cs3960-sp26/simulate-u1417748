#include "renderer.h"
#include <cmath>
#include <cstdio>
#include <cstring>

bool Renderer::init(int width, int height) {
    width_ = width;
    height_ = height;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("Physics Simulator", width, height, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

void Renderer::shutdown() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void Renderer::draw_filled_circle(float cx, float cy, float r, uint32_t color) {
    uint8_t cr = (color >> 16) & 0xFF;
    uint8_t cg = (color >> 8) & 0xFF;
    uint8_t cb = color & 0xFF;
    SDL_SetRenderDrawColor(renderer, cr, cg, cb, 255);

    int ir = (int)r;
    int x0 = (int)cx;
    int y0 = (int)cy;

    // Simple scanline fill
    for (int dy = -ir; dy <= ir; ++dy) {
        int dx = (int)std::sqrt((float)(ir * ir - dy * dy));
        SDL_RenderLine(renderer,
            (float)(x0 - dx), (float)(y0 + dy),
            (float)(x0 + dx), (float)(y0 + dy));
    }
}

void Renderer::draw(const PhysicsWorld& world, const SimMetrics& metrics,
                    bool paused, bool show_debug, float fps) {
    // Dark background
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Draw walls
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for (auto& w : world.walls) {
        SDL_RenderLine(renderer, w.a.x, w.a.y, w.b.x, w.b.y);
        // Draw walls thicker by offsetting
        SDL_RenderLine(renderer, w.a.x + 1, w.a.y, w.b.x + 1, w.b.y);
        SDL_RenderLine(renderer, w.a.x, w.a.y + 1, w.b.x, w.b.y + 1);
    }

    // Draw balls
    for (auto& b : world.balls) {
        uint32_t col = b.color;
        if (b.sleeping) {
            // Dim sleeping balls
            uint8_t r = ((col >> 16) & 0xFF) / 2;
            uint8_t g = ((col >> 8) & 0xFF) / 2;
            uint8_t bl = (col & 0xFF) / 2;
            col = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | bl;
        }
        draw_filled_circle(b.pos.x, b.pos.y, b.radius, col);
    }

    // HUD overlay
    if (show_debug) {
        // Draw debug text as colored rectangles with basic info
        // SDL3 doesn't have built-in text, so we'll draw simple status bars
        float bar_y = 10;
        char buf[256];

        // Background for HUD
        SDL_FRect hud_bg = {5, 5, 320, 160};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &hud_bg);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

        // FPS bar
        float fps_frac = std::min(fps / 120.0f, 1.0f);
        SDL_FRect fps_bar = {10, bar_y, fps_frac * 200, 8};
        SDL_RenderFillRect(renderer, &fps_bar);
        bar_y += 14;

        // KE bar (log scale)
        float ke_frac = std::min(std::log1p(metrics.total_kinetic_energy) / 20.0f, 1.0f);
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
        SDL_FRect ke_bar = {10, bar_y, ke_frac * 200, 8};
        SDL_RenderFillRect(renderer, &ke_bar);
        bar_y += 14;

        // Sleep fraction bar
        float sleep_frac = (metrics.ball_count > 0)
            ? (float)metrics.sleeping_count / metrics.ball_count : 0.0f;
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
        SDL_FRect sleep_bar = {10, bar_y, sleep_frac * 200, 8};
        SDL_RenderFillRect(renderer, &sleep_bar);
        bar_y += 14;

        // Max penetration bar
        float pen_frac = std::min(metrics.max_penetration / 5.0f, 1.0f);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect pen_bar = {10, bar_y, pen_frac * 200, 8};
        SDL_RenderFillRect(renderer, &pen_bar);
        bar_y += 14;

        // Speed bar
        float spd_frac = std::min(metrics.max_speed / 500.0f, 1.0f);
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_FRect spd_bar = {10, bar_y, spd_frac * 200, 8};
        SDL_RenderFillRect(renderer, &spd_bar);
        bar_y += 14;

        // Labels (small colored indicators)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // Paused indicator
        if (paused) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_FRect pause_ind = {10, bar_y, 30, 8};
            SDL_RenderFillRect(renderer, &pause_ind);
            SDL_FRect pause_ind2 = {50, bar_y, 30, 8};
            SDL_RenderFillRect(renderer, &pause_ind2);
        }

        // Print metrics to stdout periodically for debugging
        // (the headless mode handles structured output)
    }

    // Restitution indicator in top-right
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        float rest_bar_w = world.config.restitution * 100.0f;
        SDL_FRect rest_bar = {(float)width_ - 120, 10, rest_bar_w, 8};
        SDL_RenderFillRect(renderer, &rest_bar);
    }

    SDL_RenderPresent(renderer);
}
