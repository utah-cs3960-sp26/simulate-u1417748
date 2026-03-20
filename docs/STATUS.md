# Project Status

## Current State: ✅ Complete

All definition-of-done criteria are met.

## Quick Commands

### Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
```

### Quick inner-loop test
```bash
./build/test_physics && ./build/simulate --headless --balls 1000 --seed 1 --restitution 0.3 --frames 600 --metrics
```

### Full validation
```bash
ctest --test-dir build --output-on-failure
```

### Interactive
```bash
./build/simulate
```

## Metrics (seed=1, 1000 balls, 6000 frames)

| Restitution | Pile Height | Sleeping | Max Penetration | Max Speed |
|-------------|-------------|----------|-----------------|-----------|
| 0.1         | ~653        | 1000     | ~0.87           | 0.0       |
| 0.2         | ~656        | 1000     | ~1.12           | 0.0       |
| 0.3         | ~665        | 1000     | ~1.00           | 0.0       |
| 0.8         | ~654        | 1000     | ~1.12           | 0.0       |

Pile height variation: <3% across restitution values ✅

## Performance

- Headless: ~180-200 fps for 1000 balls (8 substeps, 4 solver iterations)
- Unit tests (incl. pile height comparison): ~5 seconds
- Full test suite: ~150 seconds

## Architecture

```
src/
├── vec2.h              # 2D vector math
├── physics.h/cpp       # Physics engine core
├── spatial_hash.h/cpp  # Spatial hash broadphase
├── scene.h/cpp         # Scene setup
├── renderer.h/cpp      # SDL3 rendering
├── main.cpp            # Interactive app + --headless mode
└── headless_main.cpp   # Standalone headless runner
tests/
└── test_physics.cpp    # Unit tests (11 test functions, 50 checks)
```

## Key Design Decisions

1. **Spatial hash broadphase** — O(n) collision detection, cell size = 4× max radius
2. **Contact pair deduplication** — pairs built once per substep, reused across solver iterations
3. **Sleeping system** — balls sleep after 0.4s below speed threshold; wake only on significant impact
4. **Separate positional correction from velocity response** — restitution doesn't affect pile geometry
5. **Hopper/funnel scene** — wide spawn area → peg rows → funnel → settling basin
6. **Unified binary** — `simulate --headless` and standalone `simulate_headless` both use `physics_core`

## Test Coverage

| Test | What it validates |
|------|------------------|
| test_vec2 | Vector math basics |
| test_gravity_single_ball | Gravity applies downward |
| test_ball_wall_collision | Ball bounces off wall correctly |
| test_ball_ball_collision | Two-ball collision separates and reverses velocity |
| test_no_nan_after_many_steps | 100 balls for 500 frames, no NaN/escape |
| test_scene_setup | 1000 balls created, walls exist, minimal initial overlaps |
| test_metrics | Metrics computation correctness |
| test_fast_wall_impact | Ball at 1500 px/s doesn't tunnel through wall |
| test_wall_corner_interaction | Ball in corner stays inside |
| test_pile_height_restitution_invariance | Low vs high restitution pile height <10% difference |
| test_stack_compression | 20-ball vertical stack settles without explosion |

## Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| Tunneling through walls | 8 substeps, speed clamping at 2000 px/s, regression test |
| NaN/Inf propagation | Guards in clamp_speeds(), checked by all headless tests |
| Pile height variation with restitution | Positional correction independent of restitution; automated <10% check |
| Jitter in resting contacts | Bounce threshold kills tiny bounces; sleep system |
| Performance with 1000 balls | Spatial hash broadphase; sleeping balls skip integration |
| Corner escape | Velocity-aware wall normal fallback; corner regression test |
| CLI input errors | Validation: substeps≥1, restitution∈[0,1], balls≥1, frames≥1 |

## Completed Work

- [x] Physics core: gravity, ball-ball, ball-wall collisions
- [x] Semi-implicit Euler integration with fixed timestep
- [x] Spatial hash broadphase
- [x] Contact pair deduplication
- [x] Iterative contact solver with separate position/velocity correction
- [x] Configurable restitution
- [x] Sleep/wake system
- [x] Friction and damping
- [x] Speed clamping and NaN guards
- [x] Hopper/funnel scene with pegs
- [x] Interactive SDL3 app with pause/resume/step/debug overlay
- [x] Headless mode with metrics and validation (both `--headless` and standalone)
- [x] Unit tests (11 test functions, 50 checks)
- [x] Headless regression tests via ctest (7 tests)
- [x] Pile height consistency across restitution values (automated)
- [x] Fast wall impact / tunneling regression test
- [x] Wall corner interaction regression test
- [x] Stack compression regression test
- [x] CLI input validation
- [x] Velocity-aware wall normal fallback for degenerate cases
- [x] README and STATUS documentation
