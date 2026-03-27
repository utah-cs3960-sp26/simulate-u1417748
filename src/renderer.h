#pragma once
#include "physics.h"
#include <SDL3/SDL.h>
#include <string>

struct Button {
    SDL_FRect rect;
    const char* label;
    uint32_t color;       // fill color (ARGB)
    uint32_t hover_color; // fill color when hovered
    bool hovered = false;

    bool contains(float mx, float my) const {
        return mx >= rect.x && mx < rect.x + rect.w &&
               my >= rect.y && my < rect.y + rect.h;
    }
};

class Renderer {
public:
    bool init(int width, int height);
    void shutdown();
    void draw(const PhysicsWorld& world, const SimMetrics& metrics,
              bool paused, bool show_debug, float fps);

    void draw_start_screen(const Button& btn_start, const Button& btn_load);
    void draw_end_screen(const std::string& saved_path, const Button& btn_show,
                         const Button& btn_restart, const Button& btn_quit);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int width_ = 1200, height_ = 800;

private:
    void draw_filled_circle(float cx, float cy, float r, uint32_t color);
    void draw_button(const Button& btn);
    void draw_bitmap_char(char c, int x, int y, int scale);
    void draw_bitmap_string(const char* text, int x, int y, int scale);
};
