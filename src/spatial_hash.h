#pragma once
#include "vec2.h"
#include <vector>
#include <cstdint>

class SpatialHash {
public:
    void clear(float cell_size, int table_size = 4096);
    void insert(int id, Vec2 pos, float radius);

    // calls callback(int other_id) for each candidate near pos±radius
    template<typename F>
    void query(Vec2 pos, float radius, F&& callback) const;

private:
    struct Entry { int id; int next; }; // next = index into entries_ or -1
    float cell_size_ = 1.0f;
    float inv_cell_ = 1.0f;
    int table_size_ = 0;
    std::vector<int> buckets_;   // head index per bucket, -1 if empty
    std::vector<Entry> entries_;

    int hash(int cx, int cy) const;
};

// --- template impl ---
template<typename F>
void SpatialHash::query(Vec2 pos, float radius, F&& callback) const {
    int x0 = (int)std::floor((pos.x - radius) * inv_cell_);
    int x1 = (int)std::floor((pos.x + radius) * inv_cell_);
    int y0 = (int)std::floor((pos.y - radius) * inv_cell_);
    int y1 = (int)std::floor((pos.y + radius) * inv_cell_);
    for (int cy = y0; cy <= y1; ++cy) {
        for (int cx = x0; cx <= x1; ++cx) {
            int h = hash(cx, cy);
            int idx = buckets_[h];
            while (idx != -1) {
                callback(entries_[idx].id);
                idx = entries_[idx].next;
            }
        }
    }
}
