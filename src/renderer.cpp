#include "renderer.h"
#include <cmath>
#include <cstdio>
#include <cstring>

// Minimal 5x7 bitmap font for uppercase, digits, and common punctuation.
// Each glyph is 5 columns x 7 rows, packed as 7 bytes (one per row, LSB = left).
static const uint8_t FONT_5X7[][7] = {
    // ' ' (space)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    // A
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    // B
    {0x0F,0x11,0x11,0x0F,0x11,0x11,0x0F},
    // C
    {0x0E,0x11,0x01,0x01,0x01,0x11,0x0E},
    // D
    {0x0F,0x11,0x11,0x11,0x11,0x11,0x0F},
    // E
    {0x1F,0x01,0x01,0x0F,0x01,0x01,0x1F},
    // F
    {0x1F,0x01,0x01,0x0F,0x01,0x01,0x01},
    // G
    {0x0E,0x11,0x01,0x1D,0x11,0x11,0x0E},
    // H
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    // I
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    // J
    {0x1C,0x08,0x08,0x08,0x08,0x09,0x06},
    // K
    {0x11,0x09,0x05,0x03,0x05,0x09,0x11},
    // L
    {0x01,0x01,0x01,0x01,0x01,0x01,0x1F},
    // M
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    // N
    {0x11,0x13,0x15,0x19,0x11,0x11,0x11},
    // O
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    // P
    {0x0F,0x11,0x11,0x0F,0x01,0x01,0x01},
    // Q
    {0x0E,0x11,0x11,0x11,0x15,0x09,0x16},
    // R
    {0x0F,0x11,0x11,0x0F,0x05,0x09,0x11},
    // S
    {0x0E,0x11,0x01,0x0E,0x10,0x11,0x0E},
    // T
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    // U
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    // V
    {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04},
    // W
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
    // X
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    // Y
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    // Z
    {0x1F,0x10,0x08,0x04,0x02,0x01,0x1F},
    // 0
    {0x0E,0x11,0x19,0x15,0x13,0x11,0x0E},
    // 1
    {0x04,0x06,0x04,0x04,0x04,0x04,0x0E},
    // 2
    {0x0E,0x11,0x10,0x08,0x04,0x02,0x1F},
    // 3
    {0x0E,0x11,0x10,0x0C,0x10,0x11,0x0E},
    // 4
    {0x08,0x0C,0x0A,0x09,0x1F,0x08,0x08},
    // 5
    {0x1F,0x01,0x0F,0x10,0x10,0x11,0x0E},
    // 6
    {0x0C,0x02,0x01,0x0F,0x11,0x11,0x0E},
    // 7
    {0x1F,0x10,0x08,0x04,0x02,0x02,0x02},
    // 8
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    // 9
    {0x0E,0x11,0x11,0x1E,0x10,0x08,0x06},
    // . (period)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x04},
    // , (comma)
    {0x00,0x00,0x00,0x00,0x00,0x04,0x02},
    // : (colon)
    {0x00,0x00,0x04,0x00,0x04,0x00,0x00},
    // / (slash)
    {0x10,0x10,0x08,0x04,0x02,0x01,0x01},
    // - (dash)
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    // _ (underscore)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x1F},
    // ( (open paren)
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08},
    // ) (close paren)
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02},
    // ! (exclamation)
    {0x04,0x04,0x04,0x04,0x04,0x00,0x04},
    // ? (question)
    {0x0E,0x11,0x10,0x08,0x04,0x00,0x04},
};

static int char_to_glyph(char c) {
    if (c == ' ') return 0;
    if (c >= 'A' && c <= 'Z') return 1 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 1 + (c - 'a'); // map lowercase → uppercase
    if (c >= '0' && c <= '9') return 27 + (c - '0');
    switch (c) {
        case '.': return 37;
        case ',': return 38;
        case ':': return 39;
        case '/': return 40;
        case '-': return 41;
        case '_': return 42;
        case '(': return 43;
        case ')': return 44;
        case '!': return 45;
        case '?': return 46;
        default: return 0; // fallback to space
    }
}

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

void Renderer::draw_bitmap_char(char c, int x, int y, int scale) {
    int idx = char_to_glyph(c);
    const uint8_t* glyph = FONT_5X7[idx];
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (glyph[row] & (1 << col)) {
                SDL_FRect px = {(float)(x + col * scale), (float)(y + row * scale),
                                (float)scale, (float)scale};
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

void Renderer::draw_bitmap_string(const char* text, int x, int y, int scale) {
    int cx = x;
    for (const char* p = text; *p; ++p) {
        draw_bitmap_char(*p, cx, y, scale);
        cx += 6 * scale; // 5 pixel width + 1 pixel gap
    }
}

void Renderer::draw_button(const Button& btn) {
    // Fill
    uint32_t c = btn.hovered ? btn.hover_color : btn.color;
    SDL_SetRenderDrawColor(renderer, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF);
    SDL_RenderFillRect(renderer, &btn.rect);

    // Border
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderRect(renderer, &btn.rect);

    // Label centered in button
    int scale = 3;
    int text_w = (int)strlen(btn.label) * 6 * scale - scale; // subtract trailing gap
    int text_h = 7 * scale;
    int tx = (int)(btn.rect.x + (btn.rect.w - text_w) / 2);
    int ty = (int)(btn.rect.y + (btn.rect.h - text_h) / 2);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    draw_bitmap_string(btn.label, tx, ty, scale);
}

void Renderer::draw_filled_circle(float cx, float cy, float r, uint32_t color) {
    uint8_t cr = (color >> 16) & 0xFF;
    uint8_t cg = (color >> 8) & 0xFF;
    uint8_t cb = color & 0xFF;
    SDL_SetRenderDrawColor(renderer, cr, cg, cb, 255);

    int ir = (int)r;
    int x0 = (int)cx;
    int y0 = (int)cy;

    for (int dy = -ir; dy <= ir; ++dy) {
        int dx = (int)std::sqrt((float)(ir * ir - dy * dy));
        SDL_RenderLine(renderer,
            (float)(x0 - dx), (float)(y0 + dy),
            (float)(x0 + dx), (float)(y0 + dy));
    }
}

void Renderer::draw_start_screen(const Button& btn_start, const Button& btn_load) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Title
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    const char* title = "PHYSICS SIMULATOR";
    int scale = 5;
    int tw = (int)strlen(title) * 6 * scale - scale;
    draw_bitmap_string(title, (width_ - tw) / 2, 150, scale);

    // Subtitle
    SDL_SetRenderDrawColor(renderer, 150, 150, 180, 255);
    const char* sub = "2D BALL SIMULATION";
    int s2 = 2;
    int sw = (int)strlen(sub) * 6 * s2 - s2;
    draw_bitmap_string(sub, (width_ - sw) / 2, 210, s2);

    // Decorative balls
    for (int i = 0; i < 20; ++i) {
        float bx = 100.0f + (float)(i * 53 % width_);
        float by = 280.0f + (float)(i * 37 % 100);
        uint32_t col = 0xFF000000u | ((i * 37 + 100) << 16) | ((i * 73 + 100) << 8) | (i * 113 + 100);
        draw_filled_circle(bx, by, 4, col);
    }

    draw_button(btn_start);
    draw_button(btn_load);

    SDL_RenderPresent(renderer);
}

void Renderer::draw_color_edit(const PhysicsWorld& world, const uint32_t* palette,
                               int palette_count, int selected_color, float brush_radius,
                               const Button& btn_done) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Draw walls
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for (auto& w : world.walls) {
        SDL_RenderLine(renderer, w.a.x, w.a.y, w.b.x, w.b.y);
        SDL_RenderLine(renderer, w.a.x + 1, w.a.y, w.b.x + 1, w.b.y);
        SDL_RenderLine(renderer, w.a.x, w.a.y + 1, w.b.x, w.b.y + 1);
    }

    // Draw balls (no sleep dimming — show true colors)
    for (auto& b : world.balls) {
        draw_filled_circle(b.pos.x, b.pos.y, b.radius, b.color);
    }

    // Brush cursor
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    for (int angle = 0; angle < 64; ++angle) {
        float a = (float)angle * 6.2832f / 64.0f;
        float px = mx + brush_radius * cosf(a);
        float py = my + brush_radius * sinf(a);
        float a2 = (float)(angle + 1) * 6.2832f / 64.0f;
        float px2 = mx + brush_radius * cosf(a2);
        float py2 = my + brush_radius * sinf(a2);
        SDL_RenderLine(renderer, px, py, px2, py2);
    }

    // Palette bar at bottom
    float swatch_size = 40.0f;
    float gap = 6.0f;
    float total_w = palette_count * (swatch_size + gap) - gap;
    float palette_x = (width_ - total_w) / 2.0f;
    float palette_y = height_ - 60.0f;

    // Background strip
    SDL_FRect pal_bg = {palette_x - 10, palette_y - 10, total_w + 20, swatch_size + 20};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &pal_bg);

    for (int i = 0; i < palette_count; ++i) {
        float sx = palette_x + i * (swatch_size + gap);
        uint32_t c = palette[i];
        SDL_SetRenderDrawColor(renderer, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 255);
        SDL_FRect swatch = {sx, palette_y, swatch_size, swatch_size};
        SDL_RenderFillRect(renderer, &swatch);

        // Selection highlight
        if (i == selected_color) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_FRect sel = {sx - 3, palette_y - 3, swatch_size + 6, swatch_size + 6};
            SDL_RenderRect(renderer, &sel);
            SDL_FRect sel2 = {sx - 2, palette_y - 2, swatch_size + 4, swatch_size + 4};
            SDL_RenderRect(renderer, &sel2);
        }
    }

    // HUD text
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    draw_bitmap_string("PAINT MODE", 10, 10, 3);

    SDL_SetRenderDrawColor(renderer, 180, 180, 200, 255);
    draw_bitmap_string("CLICK/DRAG TO PAINT   SCROLL:BRUSH SIZE   1-9:COLORS", 10, 42, 1);

    // Brush size indicator
    char buf[32];
    snprintf(buf, sizeof(buf), "BRUSH: %.0f", brush_radius);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    draw_bitmap_string(buf, width_ - 180, 10, 2);

    draw_button(btn_done);

    SDL_RenderPresent(renderer);
}

void Renderer::draw_end_screen(const std::string& saved_path, const Button& btn_show,
                               const Button& btn_restart, const Button& btn_quit) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Title
    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
    const char* title = "SIMULATION COMPLETE";
    int scale = 4;
    int tw = (int)strlen(title) * 6 * scale - scale;
    draw_bitmap_string(title, (width_ - tw) / 2, 150, scale);

    // Saved path info
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    const char* label = "RESULTS SAVED TO:";
    int s2 = 2;
    int lw = (int)strlen(label) * 6 * s2 - s2;
    draw_bitmap_string(label, (width_ - lw) / 2, 250, s2);

    // Path (may be long; show it at smaller scale)
    SDL_SetRenderDrawColor(renderer, 255, 220, 100, 255);
    int s3 = 2;
    int pw = (int)saved_path.size() * 6 * s3 - s3;
    int px = (width_ - pw) / 2;
    if (px < 20) { px = 20; s3 = 1; } // shrink if too long
    draw_bitmap_string(saved_path.c_str(), px, 285, s3);

    draw_button(btn_show);
    draw_button(btn_restart);
    draw_button(btn_quit);

    SDL_RenderPresent(renderer);
}

void Renderer::draw(const PhysicsWorld& world, const SimMetrics& metrics,
                    bool paused, bool show_debug, float fps) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Draw walls
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for (auto& w : world.walls) {
        SDL_RenderLine(renderer, w.a.x, w.a.y, w.b.x, w.b.y);
        SDL_RenderLine(renderer, w.a.x + 1, w.a.y, w.b.x + 1, w.b.y);
        SDL_RenderLine(renderer, w.a.x, w.a.y + 1, w.b.x, w.b.y + 1);
    }

    // Draw balls
    for (auto& b : world.balls) {
        uint32_t col = b.color;
        if (b.sleeping) {
            uint8_t r = ((col >> 16) & 0xFF) / 2;
            uint8_t g = ((col >> 8) & 0xFF) / 2;
            uint8_t bl = (col & 0xFF) / 2;
            col = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | bl;
        }
        draw_filled_circle(b.pos.x, b.pos.y, b.radius, col);
    }

    // HUD overlay
    if (show_debug) {
        float bar_y = 10;

        SDL_FRect hud_bg = {5, 5, 320, 175};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &hud_bg);

        // FPS bar + label
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        float fps_frac = std::min(fps / 120.0f, 1.0f);
        SDL_FRect fps_bar = {80, bar_y, fps_frac * 200, 8};
        SDL_RenderFillRect(renderer, &fps_bar);
        draw_bitmap_string("FPS", 10, (int)bar_y, 1);
        bar_y += 14;

        // KE bar
        float ke_frac = std::min(std::log1p(metrics.total_kinetic_energy) / 20.0f, 1.0f);
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
        SDL_FRect ke_bar = {80, bar_y, ke_frac * 200, 8};
        SDL_RenderFillRect(renderer, &ke_bar);
        draw_bitmap_string("KE", 10, (int)bar_y, 1);
        bar_y += 14;

        // Sleep bar
        float sleep_frac = (metrics.ball_count > 0)
            ? (float)metrics.sleeping_count / metrics.ball_count : 0.0f;
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
        SDL_FRect sleep_bar = {80, bar_y, sleep_frac * 200, 8};
        SDL_RenderFillRect(renderer, &sleep_bar);
        draw_bitmap_string("SLEEP", 10, (int)bar_y, 1);
        bar_y += 14;

        // Penetration bar
        float pen_frac = std::min(metrics.max_penetration / 5.0f, 1.0f);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect pen_bar = {80, bar_y, pen_frac * 200, 8};
        SDL_RenderFillRect(renderer, &pen_bar);
        draw_bitmap_string("PEN", 10, (int)bar_y, 1);
        bar_y += 14;

        // Speed bar
        float spd_frac = std::min(metrics.max_speed / 500.0f, 1.0f);
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_FRect spd_bar = {80, bar_y, spd_frac * 200, 8};
        SDL_RenderFillRect(renderer, &spd_bar);
        draw_bitmap_string("SPEED", 10, (int)bar_y, 1);
        bar_y += 14;

        // Paused indicator
        if (paused) {
            SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
            draw_bitmap_string("PAUSED", 10, (int)bar_y, 2);
        }

        // Controls hint
        SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
        draw_bitmap_string("SPACE:PAUSE  R:RESET  E:END  D:HUD", 10, (int)(height_ - 20), 1);
    }

    // Restitution indicator in top-right
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        float rest_bar_w = world.config.restitution * 100.0f;
        SDL_FRect rest_bar = {(float)width_ - 120, 10, rest_bar_w, 8};
        SDL_RenderFillRect(renderer, &rest_bar);
        draw_bitmap_string("REST", width_ - 120, 22, 1);
    }

    SDL_RenderPresent(renderer);
}
