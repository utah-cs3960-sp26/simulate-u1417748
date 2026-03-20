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
./build/test_physics && ./build/simulate_headless --balls 1000 --seed 1 --restitution 0.3 --frames 600 --metrics
```

### Full validation
```bash
ctest --test-dir build --output-on-failure -j4
```

### Interactive
```bash
./build/simulate
```

## Metrics (seed=1, 1000 balls, 3000 frames)

| Restitution | Pile Height | Sleeping @ 3000 | Max Penetration | Max Speed |
|-------------|-------------|-----------------|-----------------|-----------|
| 0.1         | 653.2       | 1000            | 0.87            | 0.0       |
| 0.3         | 665.0       | 1000            | 1.00            | 0.0       |
| 0.5         | 657.3       | 1000            | 0.87            | 0.0       |
| 0.8         | 653.5       | 1000            | 1.12            | 0.0       |

Pile height variation: ~2% across restitution values ✅

## Performance

- Headless: ~180-200 fps for 1000 balls (8 substeps, 4 solver iterations)
- Full test suite: ~68 seconds
- Unit tests: <1 second

## Architecture

```
src/
├── vec2.h              # 2D vector math
├── physics.h/cpp       # Physics engine core
├── spatial_hash.h/cpp  # Spatial hash broadphase
├── scene.h/cpp         # Scene setup
├── renderer.h/cpp      # SDL3 rendering
├── main.cpp            # Interactive app
└── headless_main.cpp   # Headless runner
tests/
└── test_physics.cpp    # Unit tests
```

## Key Design Decisions

1. **Spatial hash broadphase** — O(n) collision detection, cell size = 4× max radius
2. **Contact pair deduplication** — pairs built once per substep, reused across solver iterations
3. **Sleeping system** — balls sleep after 0.4s below speed threshold; wake only on significant impact
4. **Separate positional correction from velocity response** — restitution doesn't affect pile geometry
5. **Hopper/funnel scene** — wide spawn area → peg rows → funnel → settling basin

## Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| Tunneling through walls | 8 substeps, speed clamping at 2000 px/s |
| NaN/Inf propagation | Guards in clamp_speeds(), checked by all headless tests |
| Pile height variation with restitution | Positional correction independent of restitution; validated <5% |
| Jitter in resting contacts | Bounce threshold kills tiny bounces; sleep system |
| Performance with 1000 balls | Spatial hash broadphase; sleeping balls skip integration |

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
- [x] Headless mode with metrics and validation
- [x] Unit tests
- [x] Headless regression tests via ctest
- [x] Pile height consistency across restitution values
- [x] README and STATUS documentation
