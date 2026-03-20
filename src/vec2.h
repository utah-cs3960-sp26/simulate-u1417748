#pragma once
#include <cmath>
#include <limits>

struct Vec2 {
    float x = 0.0f, y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float dot(Vec2 o) const { return x * o.x + y * o.y; }
    float cross(Vec2 o) const { return x * o.y - y * o.x; }
    float length_sq() const { return x * x + y * y; }
    float length() const { return std::sqrt(length_sq()); }

    Vec2 normalized() const {
        float len = length();
        if (len < 1e-12f) return {0, 0};
        return *this / len;
    }

    bool is_finite() const {
        return std::isfinite(x) && std::isfinite(y);
    }
};

inline Vec2 operator*(float s, Vec2 v) { return {s * v.x, s * v.y}; }
