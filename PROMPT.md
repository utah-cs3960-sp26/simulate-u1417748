# Autonomous project brief: SDL3 C++ 2D physics simulator

You are working autonomously in this repository to build the project to completion.
The human will steer you by editing **PROMPT.md** and re-running you in a loop.
**Do not edit `PROMPT.md` yourself.** Everything else in the repo is fair game.

## Mission

Build a submission-ready **2D physics simulator** in **C++ using SDL3**.

The simulator must:
- simulate **circular balls** and **fixed, immovable walls**
- apply **gravity**
- support **ball-ball** and **ball-wall** collisions
- use **non-elastic collisions with configurable restitution**
- launch into an initial scene with **about 1000 balls** inside a container made from wall pieces
- be interesting to watch at startup, with balls bouncing around for a while and then settling
- be stable: balls should not obviously overlap, squeeze through walls, jitter forever, or accelerate into nonsense / infinity

Do **not** use an external physics engine. Implement the physics in this repository.

The target is not perfect real-world accuracy. The target is a robust, plausible, performant simulator that satisfies the assignment.

## Definition of done

The project is done when all of the following are true:

- The repo builds from a fresh checkout with documented steps.
- The app uses **SDL3 + C++** and opens a window showing the simulator.
- The default scene contains roughly **1000 balls** in a wall-built container.
- Gravity, ball-ball collisions, and ball-wall collisions work.
- Restitution is configurable at runtime and/or via CLI.
- Balls do not visibly tunnel through walls or remain deeply interpenetrating.
- Long-run instability is controlled: no NaNs, no explosions, no runaway speed growth.
- The final settled pile occupies effectively the **same amount of space across low vs high restitution**, within a reasonable tolerance for the same seed/scene.
- The same physics core powers both:
  - the normal interactive SDL app
  - a deterministic **headless experiment / validation mode**
- There are reproducible regression checks for the tricky cases.
- `README.md` documents build, run, controls, and validation.
- `docs/STATUS.md` tracks progress, commands, metrics, risks, and next steps.
- The git history contains meaningful commits along the way.

Leave the repo **push-ready**.

## Highest priorities

Prioritize work in this order:

1. **Correctness and stability**
2. **Fast feedback loops for debugging**
3. **Determinism and reproducible experiments**
4. **Performance good enough for ~1000 balls**
5. **Visual polish**

Prefer a simple, robust design over an elaborate one.

## Make it feedback-loopable

Follow the spirit of the “Feedback Loopable” workflow:

### 1) Build a playground
Create an interactive SDL app that makes it easy to inspect the simulation. Add, if easy and useful:
- pause/resume
- reset scene
- single-step
- overlay of metrics such as FPS, ball count, max penetration, sleeping count, restitution, etc.

### 2) Build experiments
Expose key parameters via CLI flags so failures are reproducible. At minimum make scene/setup, seed, restitution, ball count, frame count, timestep/substeps, and any debug/snapshot output configurable.

Use a **fixed default seed** so the default behavior is reproducible unless overridden.

### 3) Make the inner loop fast
Create a **headless mode** that runs the exact same physics engine without opening a window and reports useful text metrics to stdout. If helpful, let it also dump a snapshot image and/or CSV/JSON metrics.

The point is to let yourself debug by running very fast repeatable experiments, not only by staring at a live animation.

Whenever a bug appears:
- create the smallest deterministic reproducer you can
- fix the bug
- keep the reproducer as a regression check if it is useful

When in doubt, make the simulator **more feedback-loopable** instead of guessing.

## Preferred architecture

Unless the repo already strongly suggests another good structure, standardize on:

- **CMake** build system
- **C++17 or newer**
- one shared physics core/library used by both the interactive app and the headless runner
- minimal dependencies beyond **SDL3** and the standard library

Suggested shape (adapt if needed):
- `src/` for physics, rendering, app code
- `include/` for headers if useful
- `tests/` and/or headless regression code
- `docs/STATUS.md` for rolling project memory
- `README.md` for user-facing instructions

Respect existing good work. Extend it rather than rewriting unless a rewrite is clearly cheaper and safer.

## Physics guidance

Choose the implementation details yourself, but bias toward methods that remain stable for many interacting balls.

Strongly preferred approaches:
- **fixed timestep** physics, not variable-dt physics
- **semi-implicit Euler** or another simple stable integrator
- **multiple substeps** per frame if needed
- circle-circle narrowphase for ball-ball collisions
- segment/edge walls for static boundaries
- closest-point circle-vs-segment tests for ball-wall collisions
- **uniform grid / spatial hash broadphase** so ~1000 balls is comfortable
- **iterative contact solving**
- **separate positional correction from velocity response**
- restitution affecting bounce response, **not** positional correction / resting contact resolution

Important stability strategies you may use if helpful:
- penetration slop and capped positional correction
- bounce threshold so tiny resting impacts do not keep re-bouncing
- sleeping / wake-up logic
- mild damping and/or friction if needed for stability and settling
- conservative substeps or swept handling for fast wall impacts
- sane assertions / guards for NaN, Inf, and absurd speeds

It is acceptable to use friction and/or a small damping term if it materially improves stability, but do not let restitution become the thing that changes the final packed volume. The settled pile’s occupied extent should be nearly the same for different restitution values on the same seed.

## Scene and UX expectations

Create a default scene that is visually interesting but stable:
- around **1000 balls**
- a container built from **wall pieces**
- enough motion to be pleasant to watch before settling
- a shape that naturally produces a packed resting state

A simple bowl / hopper / box with sloped walls and optional internal ramps or pegs is perfectly acceptable.

Keep rendering simple. Correctness matters more than fancy visuals.

Document controls in `README.md`. Prefer having at least:
- pause/resume
- reset
- single-step
- restitution control or presets
- optional debug overlay toggle

## Validation requirements

Do not rely on visual inspection alone. Build fast, repeatable validation.

At minimum, create and routinely run:

- a normal build
- automated tests / regression checks via `ctest` or an equivalent repeatable command
- one or more headless runs that report and/or assert:
  - no NaN / Inf state
  - no escaped balls
  - bounded maximum penetration
  - bounded maximum speed
  - no long-run energy blow-up / numerical explosion
  - a settling metric (for example kinetic energy and/or sleeping count)
  - final occupied extent / pile height for the same scene under multiple restitution values
  - acceptable performance for ~1000 balls

Useful regression scenarios to preserve:
- dense pile settling in the main container
- fast wall impact / tunneling risk
- wall junction / corner interaction
- stack compression / resting contact stability
- long-run stability soak test
- same seed + scene with low and high restitution, comparing settled occupied height/extent

Prefer making headless regressions invokable through `ctest`.

Keep the current **quick inner-loop command** and the **full validation command** written down in `docs/STATUS.md`.

## Build and environment

Use **SDL3 via C++**. Do not switch libraries.

If SDL3 is not already wired up:
- prefer the simplest robust path first
- use system SDL3 if available and reliable in the environment
- otherwise set up a clean CMake-based fallback that still clearly uses SDL3
- document the exact working build/run steps in `README.md`

Keep warnings high and fix meaningful warnings.
Use sanitizers in debug builds if the environment supports them.

Also:
- do not commit build artifacts
- do not commit `amp.log`
- update `.gitignore` if needed

## Target commands

Aim to make commands roughly like these work, then record the exact equivalents you chose:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simulate
./build/simulate --headless --scene main --balls 1000 --seed 1 --restitution 0.2 --frames 6000 --metrics