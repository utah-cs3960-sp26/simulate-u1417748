#include "physics.h"
#include <cmath>
#include <algorithm>
#include <cassert>
#include <utility>

static Vec2 closest_point_on_segment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = b - a;
    float len2 = ab.length_sq();
    if (len2 < 1e-12f) return a;
    float t = (p - a).dot(ab) / len2;
    t = std::clamp(t, 0.0f, 1.0f);
    return a + ab * t;
}

void PhysicsWorld::step() {
    float sub_dt = config.dt / config.substeps;
    for (int s = 0; s < config.substeps; ++s) {
        substep(sub_dt);
    }
}

void PhysicsWorld::substep(float sub_dt) {
    integrate(sub_dt);
    build_grid();
    build_contact_pairs();
    for (int iter = 0; iter < config.solver_iterations; ++iter) {
        solve_contacts(sub_dt);
    }
    clamp_speeds();
    update_sleep(sub_dt);
}

void PhysicsWorld::integrate(float sub_dt) {
    Vec2 gravity_accel = {0.0f, config.gravity};
    for (auto& b : balls) {
        if (b.sleeping) continue;
        // Semi-implicit Euler: update velocity first, then position
        b.vel += gravity_accel * sub_dt;
        b.vel *= config.damping;
        b.pos += b.vel * sub_dt;
    }
}

void PhysicsWorld::build_grid() {
    // Find max radius for cell size
    max_radius_ = 0.0f;
    for (auto& b : balls) max_radius_ = std::max(max_radius_, b.radius);
    float cell = std::max(max_radius_ * 4.0f, 1.0f);
    grid_.clear(cell, 4096);
    for (int i = 0; i < (int)balls.size(); ++i) {
        grid_.insert(i, balls[i].pos, balls[i].radius);
    }
}

void PhysicsWorld::build_contact_pairs() {
    contact_pairs_.clear();
    int n = (int)balls.size();
    constexpr float margin = 0.5f;

    for (int i = 0; i < n; ++i) {
        Ball& bi = balls[i];
        float query_r = bi.radius + max_radius_;
        grid_.query(bi.pos, query_r, [&](int j) {
            if (j <= i) return;
            float dist2 = (balls[j].pos - bi.pos).length_sq();
            float min_dist = bi.radius + balls[j].radius + margin;
            if (dist2 < min_dist * min_dist) {
                contact_pairs_.emplace_back(i, j);
            }
        });
    }

    // Deduplicate (multi-cell insertion can produce duplicates)
    std::sort(contact_pairs_.begin(), contact_pairs_.end());
    contact_pairs_.erase(
        std::unique(contact_pairs_.begin(), contact_pairs_.end()),
        contact_pairs_.end());
}

void PhysicsWorld::solve_contacts(float sub_dt) {
    for (auto& [i, j] : contact_pairs_) {
        solve_ball_ball(i, j, sub_dt);
    }

    int n = (int)balls.size();
    for (int i = 0; i < n; ++i) {
        for (auto& w : walls) {
            solve_ball_wall(i, w, sub_dt);
        }
    }
}

void PhysicsWorld::solve_ball_ball(int i, int j, float sub_dt) {
    Ball& a = balls[i];
    Ball& b = balls[j];

    Vec2 delta = b.pos - a.pos;
    float dist2 = delta.length_sq();
    float min_dist = a.radius + b.radius;

    if (dist2 >= min_dist * min_dist) return;

    // If both balls are sleeping, skip entirely
    if (a.sleeping && b.sleeping) return;

    if (dist2 < 1e-12f) {
        delta = {0.01f, 0.0f};
        dist2 = delta.length_sq();
    }

    float dist = std::sqrt(dist2);
    Vec2 normal = delta / dist;
    float penetration = min_dist - dist;

    // Compute relative approach speed to decide wake
    Vec2 rel_vel = b.vel - a.vel;
    float vel_along_normal = rel_vel.dot(normal);
    float approach_speed = std::abs(vel_along_normal);

    // Wake sleeping balls only if approach speed exceeds threshold
    if (a.sleeping && approach_speed > config.wake_threshold) {
        a.sleeping = false; a.sleep_timer = 0.0f;
    }
    if (b.sleeping && approach_speed > config.wake_threshold) {
        b.sleeping = false; b.sleep_timer = 0.0f;
    }

    // Effective inverse masses: sleeping (not woken) balls act as static
    float a_inv = a.sleeping ? 0.0f : a.inv_mass;
    float b_inv = b.sleeping ? 0.0f : b.inv_mass;

    // --- Positional correction ---
    float corr = penetration - config.slop;
    if (corr > 0.0f) {
        corr = std::min(corr * config.correction_frac, config.max_correction);
        float total_inv = a_inv + b_inv;
        if (total_inv > 0.0f) {
            a.pos -= normal * (corr * a_inv / total_inv);
            b.pos += normal * (corr * b_inv / total_inv);
        }
    }

    // --- Velocity response ---
    if (vel_along_normal > 0.0f) return; // separating

    float e = (approach_speed < config.bounce_threshold)
              ? 0.0f : config.restitution;

    float total_inv = a_inv + b_inv;
    if (total_inv < 1e-12f) return;

    float j_n = -(1.0f + e) * vel_along_normal / total_inv;
    Vec2 impulse = normal * j_n;

    a.vel -= impulse * a_inv;
    b.vel += impulse * b_inv;

    // Tangential friction
    Vec2 tangent = rel_vel - normal * vel_along_normal;
    float tan_len = tangent.length();
    if (tan_len > 1e-6f) {
        tangent = tangent / tan_len;
        float j_t = -rel_vel.dot(tangent) / total_inv;
        float max_friction = config.friction * std::abs(j_n);
        j_t = std::clamp(j_t, -max_friction, max_friction);
        Vec2 friction_impulse = tangent * j_t;
        a.vel -= friction_impulse * a_inv;
        b.vel += friction_impulse * b_inv;
    }
}

void PhysicsWorld::solve_ball_wall(int i, const Wall& w, float sub_dt) {
    Ball& ball = balls[i];

    Vec2 closest = closest_point_on_segment(ball.pos, w.a, w.b);
    Vec2 delta = ball.pos - closest;
    float dist2 = delta.length_sq();

    if (dist2 >= ball.radius * ball.radius) return;
    if (dist2 < 1e-12f) {
        // Ball center exactly on wall - push away from wall
        // Use velocity to pick the correct side; if velocity is zero,
        // use the wall's left-hand normal (consistent for CW containers).
        Vec2 wall_dir = (w.b - w.a).normalized();
        Vec2 perp = {-wall_dir.y, wall_dir.x}; // one perpendicular
        // Pick side: if ball is moving into the wall, push opposite to velocity component
        float vel_dot = ball.vel.dot(perp);
        if (std::abs(vel_dot) > 1e-6f) {
            delta = (vel_dot < 0.0f) ? perp * -1.0f : perp;
        } else {
            // Fallback: push upward (away from gravity) or use perp
            delta = (perp.y < 0.0f) ? perp : perp * -1.0f;
        }
        dist2 = 1e-6f;
    }

    float dist = std::sqrt(dist2);
    Vec2 normal = delta / dist;
    float penetration = ball.radius - dist;

    // Check approach speed for wake decision
    float vel_along_normal = ball.vel.dot(normal);
    float approach_speed = std::abs(vel_along_normal);

    if (ball.sleeping && approach_speed > config.wake_threshold) {
        ball.sleeping = false;
        ball.sleep_timer = 0.0f;
    }

    // Positional correction (always, even if sleeping)
    float corr = penetration - config.slop;
    if (corr > 0.0f) {
        corr = std::min(corr * config.correction_frac, config.max_correction);
        ball.pos += normal * corr;
    }

    // Skip velocity response if still sleeping
    if (ball.sleeping) return;

    // Velocity response
    if (vel_along_normal > 0.0f) return; // moving away from wall

    float e = (approach_speed < config.bounce_threshold)
              ? 0.0f : config.restitution;

    ball.vel -= normal * ((1.0f + e) * vel_along_normal);

    // Tangential friction
    Vec2 tangent = ball.vel - normal * ball.vel.dot(normal);
    float tan_speed = tangent.length();
    if (tan_speed > 1e-6f) {
        float friction_decel = config.friction * std::abs((1.0f + e) * vel_along_normal);
        float scale = std::max(0.0f, 1.0f - friction_decel / tan_speed);
        ball.vel = normal * ball.vel.dot(normal) + tangent * scale;
    }
}

void PhysicsWorld::clamp_speeds() {
    for (auto& b : balls) {
        if (b.sleeping) continue;
        float spd2 = b.vel.length_sq();
        if (spd2 > config.max_speed * config.max_speed) {
            b.vel = b.vel.normalized() * config.max_speed;
        }
        if (!b.vel.is_finite() || !b.pos.is_finite()) {
            b.vel = {0, 0};
            b.pos = {0, 0};
        }
    }
}

void PhysicsWorld::update_sleep(float sub_dt) {
    for (auto& b : balls) {
        if (b.sleeping) {
            if (b.vel.length_sq() > config.wake_threshold * config.wake_threshold) {
                b.sleeping = false;
                b.sleep_timer = 0.0f;
            }
            continue;
        }
        float speed = b.vel.length();
        if (speed < config.sleep_threshold) {
            b.sleep_timer += sub_dt;
            if (b.sleep_timer > config.sleep_time) {
                b.sleeping = true;
                b.vel = {0, 0};
            }
        } else {
            b.sleep_timer = 0.0f;
        }
    }
}

SimMetrics PhysicsWorld::compute_metrics() const {
    SimMetrics m;
    m.ball_count = (int)balls.size();
    m.max_speed = 0;
    m.max_penetration = 0;
    m.total_kinetic_energy = 0;
    m.has_nan = false;
    m.escaped_count = 0;
    m.sleeping_count = 0;
    m.min_y = 1e9f;
    m.max_y = -1e9f;

    // Build a temporary spatial hash for O(n) penetration checks
    float max_r = 0.0f;
    for (auto& b : balls) max_r = std::max(max_r, b.radius);
    SpatialHash tmp_grid;
    float cell = std::max(max_r * 4.0f, 1.0f);
    tmp_grid.clear(cell, 4096);
    for (int i = 0; i < (int)balls.size(); ++i) {
        tmp_grid.insert(i, balls[i].pos, balls[i].radius);
    }

    for (int i = 0; i < (int)balls.size(); ++i) {
        const Ball& b = balls[i];
        if (!b.pos.is_finite() || !b.vel.is_finite()) {
            m.has_nan = true;
        }
        if (b.sleeping) m.sleeping_count++;
        float spd = b.vel.length();
        m.max_speed = std::max(m.max_speed, spd);
        float mass = (b.inv_mass > 0) ? 1.0f / b.inv_mass : 0.0f;
        m.total_kinetic_energy += 0.5f * mass * spd * spd;

        if (b.pos.x < bound_left || b.pos.x > bound_right ||
            b.pos.y < bound_top || b.pos.y > bound_bottom) {
            m.escaped_count++;
        }

        m.min_y = std::min(m.min_y, b.pos.y - b.radius);
        m.max_y = std::max(m.max_y, b.pos.y + b.radius);

        // Check ball-ball penetration via spatial hash
        float query_r = b.radius + max_r;
        tmp_grid.query(b.pos, query_r, [&](int j) {
            if (j <= i) return;
            const Ball& b2 = balls[j];
            float dist = (b.pos - b2.pos).length();
            float pen = (b.radius + b2.radius) - dist;
            if (pen > 0) m.max_penetration = std::max(m.max_penetration, pen);
        });
    }

    // Check ball-wall penetration
    for (auto& b : balls) {
        for (auto& w : walls) {
            Vec2 cp = closest_point_on_segment(b.pos, w.a, w.b);
            float dist = (b.pos - cp).length();
            float pen = b.radius - dist;
            if (pen > 0) m.max_penetration = std::max(m.max_penetration, pen);
        }
    }

    m.pile_height = m.max_y - m.min_y;
    return m;
}


