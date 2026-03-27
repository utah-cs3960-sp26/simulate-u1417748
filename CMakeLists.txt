cmake_minimum_required(VERSION 3.20)
project(PhysicsSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- SDL3 via FetchContent ---
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.14
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# --- Physics library (shared between interactive + headless) ---
add_library(physics_core STATIC
    src/physics.cpp
    src/scene.cpp
    src/spatial_hash.cpp
)
target_include_directories(physics_core PUBLIC src)

# --- Interactive SDL app ---
add_executable(simulate src/main.cpp src/renderer.cpp)
target_link_libraries(simulate PRIVATE physics_core SDL3::SDL3-static)

# --- Headless runner ---
add_executable(simulate_headless src/headless_main.cpp)
target_link_libraries(simulate_headless PRIVATE physics_core)

# --- Colorize tool (assigns image colors to balls based on final positions) ---
add_executable(colorize src/colorize.cpp)
target_link_libraries(colorize PRIVATE physics_core)

# --- Tests ---
enable_testing()
add_executable(test_physics tests/test_physics.cpp)
target_link_libraries(test_physics PRIVATE physics_core)
add_test(NAME unit_tests COMMAND test_physics)

# Headless regression tests
add_test(NAME headless_main_default
    COMMAND simulate_headless --scene main --balls 1000 --seed 1 --restitution 0.3 --frames 3000 --metrics)
add_test(NAME headless_low_restitution
    COMMAND simulate_headless --scene main --balls 1000 --seed 1 --restitution 0.1 --frames 6000 --metrics)
add_test(NAME headless_high_restitution
    COMMAND simulate_headless --scene main --balls 1000 --seed 1 --restitution 0.8 --frames 6000 --metrics)
add_test(NAME headless_tunneling_stress
    COMMAND simulate_headless --scene main --balls 500 --seed 42 --restitution 0.5 --frames 2000 --metrics)
add_test(NAME headless_long_soak
    COMMAND simulate_headless --scene main --balls 1000 --seed 1 --restitution 0.3 --frames 12000 --metrics)

# Also test via unified binary --headless flag
add_test(NAME headless_via_simulate
    COMMAND simulate --headless --scene main --balls 500 --seed 1 --restitution 0.3 --frames 1000 --metrics)
