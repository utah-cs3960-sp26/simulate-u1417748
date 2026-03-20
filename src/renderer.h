#pragma once
#include "physics.h"
#include <SDL3/SDL.h>

class Renderer {
public:
    bool init(int width, int height);
    void shutdown();
    void draw(const PhysicsWorld& world, const SimMetrics& metrics,
              bool paused, bool show_debug, float fps);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int width_ = 1200, height_ = 800;

private:
    void draw_circle(float cx, float cy, float r, uint32_t color);
    void draw_filled_circle(float cx, float cy, float r, uint32_t color);
    void draw_text(const char* text, int x, int y);
};
