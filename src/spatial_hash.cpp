#include "spatial_hash.h"
#include <cmath>

void SpatialHash::clear(float cell_size, int table_size) {
    cell_size_ = cell_size;
    inv_cell_ = 1.0f / cell_size;
    table_size_ = table_size;
    buckets_.assign(table_size, -1);
    entries_.clear();
}

void SpatialHash::insert(int id, Vec2 pos, float radius) {
    int x0 = (int)std::floor((pos.x - radius) * inv_cell_);
    int x1 = (int)std::floor((pos.x + radius) * inv_cell_);
    int y0 = (int)std::floor((pos.y - radius) * inv_cell_);
    int y1 = (int)std::floor((pos.y + radius) * inv_cell_);
    for (int cy = y0; cy <= y1; ++cy) {
        for (int cx = x0; cx <= x1; ++cx) {
            int h = hash(cx, cy);
            int idx = (int)entries_.size();
            entries_.push_back({id, buckets_[h]});
            buckets_[h] = idx;
        }
    }
}

int SpatialHash::hash(int cx, int cy) const {
    // Simple spatial hash
    uint32_t h = (uint32_t)(cx * 92837111) ^ (uint32_t)(cy * 689287499);
    return (int)(h % (uint32_t)table_size_);
}
