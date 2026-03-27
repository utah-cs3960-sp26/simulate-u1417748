#include "physics.h"
#include "scene.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>

// Minimal BMP loader — supports 24-bit and 32-bit uncompressed BMPs.
struct Image {
    int width = 0, height = 0;
    std::vector<uint32_t> pixels; // ARGB, row-major, top-to-bottom

    bool load_bmp(const char* path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;

        uint8_t header[54];
        f.read(reinterpret_cast<char*>(header), 54);
        if (!f || header[0] != 'B' || header[1] != 'M') return false;

        auto read32 = [&](int off) -> uint32_t {
            return (uint32_t)header[off] | ((uint32_t)header[off+1] << 8)
                 | ((uint32_t)header[off+2] << 16) | ((uint32_t)header[off+3] << 24);
        };
        auto read16 = [&](int off) -> uint16_t {
            return (uint16_t)header[off] | ((uint16_t)header[off+1] << 8);
        };

        uint32_t data_offset = read32(10);
        width = (int)read32(18);
        height = (int)read32(22);
        uint16_t bpp = read16(28);
        uint32_t compression = read32(30);

        if (compression != 0 || (bpp != 24 && bpp != 32)) return false;

        bool bottom_up = (height > 0);
        if (height < 0) height = -height;

        pixels.resize(width * height);
        int bytes_per_pixel = bpp / 8;
        int row_size = ((width * bytes_per_pixel + 3) / 4) * 4; // padded to 4 bytes
        std::vector<uint8_t> row_buf(row_size);

        f.seekg(data_offset);
        for (int y = 0; y < height; ++y) {
            f.read(reinterpret_cast<char*>(row_buf.data()), row_size);
            if (!f) return false;
            int dst_y = bottom_up ? (height - 1 - y) : y;
            for (int x = 0; x < width; ++x) {
                int off = x * bytes_per_pixel;
                uint8_t b = row_buf[off];
                uint8_t g = row_buf[off + 1];
                uint8_t r = row_buf[off + 2];
                pixels[dst_y * width + x] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
            }
        }
        return true;
    }

    uint32_t sample(float fx, float fy) const {
        int x = std::clamp((int)fx, 0, width - 1);
        int y = std::clamp((int)fy, 0, height - 1);
        return pixels[y * width + x];
    }
};

static void print_usage() {
    printf("Usage: colorize [options]\n");
    printf("  --input CSV       Input scene CSV (required)\n");
    printf("  --image BMP       Image file for color sampling (required, BMP format)\n");
    printf("  --output CSV      Output scene CSV with assigned colors (required)\n");
    printf("  --frames N        Frames to simulate before sampling (default: 3000)\n");
    printf("  --restitution F   Coefficient of restitution (default: 0.3)\n");
    printf("  --substeps N      Physics substeps per frame (default: 8)\n");
    printf("  --help            Show this help\n");
    printf("\nThis tool loads an initial scene from a CSV, simulates physics,\n");
    printf("then assigns each ball a color by sampling the image at its final position.\n");
    printf("The output CSV has the original starting positions with the new colors.\n");
}

int main(int argc, char* argv[]) {
    std::string input_csv;
    std::string image_path;
    std::string output_csv;
    int frames = 3000;
    float restitution = 0.3f;
    int substeps = 8;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            input_csv = argv[++i];
        else if (strcmp(argv[i], "--image") == 0 && i + 1 < argc)
            image_path = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output_csv = argv[++i];
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
            frames = std::max(1, atoi(argv[++i]));
        else if (strcmp(argv[i], "--restitution") == 0 && i + 1 < argc)
            restitution = std::clamp((float)atof(argv[++i]), 0.0f, 1.0f);
        else if (strcmp(argv[i], "--substeps") == 0 && i + 1 < argc)
            substeps = std::max(1, atoi(argv[++i]));
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
    }

    if (input_csv.empty() || image_path.empty() || output_csv.empty()) {
        fprintf(stderr, "Error: --input, --image, and --output are all required.\n\n");
        print_usage();
        return 1;
    }

    // Load the image
    Image img;
    if (!img.load_bmp(image_path.c_str())) {
        fprintf(stderr, "Error: failed to load BMP image: %s\n", image_path.c_str());
        return 1;
    }
    printf("Loaded image: %dx%d\n", img.width, img.height);

    // Load the initial scene
    PhysicsWorld world;
    world.config.substeps = substeps;
    if (!load_scene_csv(world, input_csv, restitution)) {
        fprintf(stderr, "Error: failed to load scene CSV: %s\n", input_csv.c_str());
        return 1;
    }
    printf("Loaded %d balls from %s\n", (int)world.balls.size(), input_csv.c_str());

    // Save initial state
    std::vector<Vec2> initial_positions;
    std::vector<Vec2> initial_velocities;
    std::vector<float> initial_radii;
    initial_positions.reserve(world.balls.size());
    initial_velocities.reserve(world.balls.size());
    initial_radii.reserve(world.balls.size());
    for (const auto& b : world.balls) {
        initial_positions.push_back(b.pos);
        initial_velocities.push_back(b.vel);
        initial_radii.push_back(b.radius);
    }

    // Run simulation
    printf("Simulating %d frames...\n", frames);
    for (int f = 0; f < frames; ++f) {
        world.step();
        if (f % 500 == 0) {
            printf("  frame %d/%d\n", f, frames);
        }
    }
    printf("Simulation complete.\n");

    // Sample colors from image at final ball positions and write output
    // Map simulation coords to image coords: the simulation is 1200x800,
    // so we scale ball positions to the image dimensions.
    float scale_x = (float)img.width / 1200.0f;
    float scale_y = (float)img.height / 800.0f;

    std::ofstream out(output_csv);
    if (!out.is_open()) {
        fprintf(stderr, "Error: cannot open output file: %s\n", output_csv.c_str());
        return 1;
    }

    out << "x,y,vx,vy,radius,color\n";
    for (size_t i = 0; i < world.balls.size(); ++i) {
        // Sample at final position, scaled to image space
        float img_x = world.balls[i].pos.x * scale_x;
        float img_y = world.balls[i].pos.y * scale_y;
        uint32_t color = img.sample(img_x, img_y);
        uint32_t rgb = color & 0x00FFFFFFu;

        // Write with original starting position/velocity but new color
        out << initial_positions[i].x << "," << initial_positions[i].y << ","
            << initial_velocities[i].x << "," << initial_velocities[i].y << ","
            << initial_radii[i] << ","
            << std::uppercase << std::hex << std::setfill('0') << std::setw(6) << rgb
            << std::dec << "\n";
    }
    out.close();

    printf("Wrote colorized scene to %s (%d balls)\n", output_csv.c_str(), (int)world.balls.size());
    return 0;
}
