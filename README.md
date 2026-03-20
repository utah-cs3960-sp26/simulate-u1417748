# 2D Physics Simulator

A C++ 2D physics simulator using SDL3 featuring circular balls with gravity, ball-ball collisions, ball-wall collisions, and configurable restitution. Simulates ~1000 balls in a hopper/funnel container.

## Build

Requires CMake ≥ 3.20 and a C++17 compiler. SDL3 is fetched automatically via FetchContent.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
```

## Run

### Interactive Mode

```bash
./build/simulate
./build/simulate --balls 1000 --seed 1 --restitution 0.3
./build/simulate --balls 500 --seed 42 --restitution 0.8 --substeps 8
```

### Headless Mode

Run the same physics without a window, printing metrics:

```bash
./build/simulate_headless --scene main --balls 1000 --seed 1 --restitution 0.3 --frames 6000 --metrics
./build/simulate_headless --verbose --frames 3000  # periodic status every 100 frames
```

### CLI Flags

| Flag            | Default | Description                            |
|-----------------|---------|----------------------------------------|
| `--balls N`     | 1000    | Number of balls                        |
| `--seed N`      | 1       | RNG seed for deterministic runs        |
| `--restitution F` | 0.3   | Coefficient of restitution [0..1]      |
| `--scene NAME`  | main    | Scene name                             |
| `--substeps N`  | 8       | Physics substeps per frame             |
| `--frames N`    | 3000    | Frames to simulate (headless only)     |
| `--metrics`     | off     | Print final metrics (headless only)    |
| `--verbose`     | off     | Print periodic metrics (headless only) |

## Controls (Interactive)

| Key         | Action                        |
|-------------|-------------------------------|
| Space       | Pause / Resume                |
| R           | Reset scene                   |
| S           | Single step (when paused)     |
| D           | Toggle debug overlay          |
| Up / Down   | Increase / Decrease restitution by 0.05 |
| Q / Escape  | Quit                          |

## Debug Overlay

When enabled (press D), the overlay shows:
- **Green bar**: FPS
- **Orange bar**: Kinetic energy (log scale)
- **Blue bar**: Sleep fraction
- **Red bar**: Max penetration
- **Yellow bar**: Max speed
- **Top-right**: Restitution level
- **Red rectangles**: Paused indicator

## Validation

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

Tests include:
- **unit_tests**: Vec2, gravity, ball-wall bounce, ball-ball collision, NaN checks, scene setup
- **headless_main_default**: 1000 balls, 3000 frames, restitution 0.3
- **headless_low_restitution**: 1000 balls, 6000 frames, restitution 0.1
- **headless_high_restitution**: 1000 balls, 6000 frames, restitution 0.8
- **headless_tunneling_stress**: 500 balls, 2000 frames, seed 42
- **headless_long_soak**: 1000 balls, 12000 frames, stability soak

Each headless test validates: no NaN/Inf, no escaped balls, bounded penetration (<5), bounded speed (<1500).

## Architecture

- `src/physics.h/cpp` — Physics engine core (integrator, collision solver, spatial hash broadphase, sleeping)
- `src/spatial_hash.h/cpp` — Spatial hash grid for O(n) broadphase collision detection
- `src/scene.h/cpp` — Scene setup (container walls, ball spawning, RNG)
- `src/renderer.h/cpp` — SDL3 rendering
- `src/main.cpp` — Interactive SDL application
- `src/headless_main.cpp` — Headless validation/experiment runner
- `src/vec2.h` — 2D vector math
- `tests/test_physics.cpp` — Unit tests

## Physics Implementation

- **Fixed timestep** with semi-implicit Euler integration
- **8 substeps** per frame for stability
- **Spatial hash broadphase** for O(n) collision detection
- **Iterative contact solver** (4 iterations) with separate positional correction and velocity response
- **Penetration slop** and capped positional correction
- **Bounce threshold** — tiny resting impacts don't bounce
- **Sleep/wake system** — balls sleep when slow, wake on significant impact
- **Friction** and velocity damping for settling
- **Speed clamping** and NaN guards
