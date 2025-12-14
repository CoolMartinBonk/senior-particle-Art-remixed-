#pragma once
#include "GameConfig.h"
#include <vector>
#include <cmath>
#include <algorithm>

class SpatialGrid {
public:
    std::vector<int> cellStart;
    std::vector<int> cellCount;

    int cols, rows;
    float cellSize;
    float invCellSize;

    SpatialGrid(float width, float height, float size) {
        cellSize = size;
        invCellSize = 1.0f / size;
        resize(width, height);
    }

    int get_cols() const { return cols; }
    int get_rows() const { return rows; }

    void resize(float width, float height) {
        cols = static_cast<int>(std::ceil(width * invCellSize)) + 1;
        rows = static_cast<int>(std::ceil(height * invCellSize)) + 1;

        int cellNum = cols * rows;
        cellStart.assign(cellNum, 0);
        cellCount.assign(cellNum, 0);
    }

    std::vector<int> get_active_keys() const {
        std::vector<int> active_keys;
        active_keys.reserve(TOTAL_PARTICLES / 2);

        for (size_t i = 0; i < cellCount.size(); ++i) {
            if (cellCount[i] > 0) {
                active_keys.push_back((int)i);
            }
        }
        return active_keys;
    }

    void update_and_sort(std::vector<Particle>& particles, std::vector<Particle>& buffer) {
        int cellNum = cols * rows;

        if (cellCount.size() != cellNum) {
            cellCount.resize(cellNum);
            cellStart.resize(cellNum);
        }

        std::fill(cellCount.begin(), cellCount.end(), 0);

        for (const auto& p : particles) {
            int cx = (int)(p.x * invCellSize);
            int cy = (int)(p.y * invCellSize);

            if (cx < 0) cx = 0; else if (cx >= cols) cx = cols - 1;
            if (cy < 0) cy = 0; else if (cy >= rows) cy = rows - 1;

            cellCount[cx + cy * cols]++;
        }

        int offset = 0;
        for (int i = 0; i < cellNum; ++i) {
            cellStart[i] = offset;
            offset += cellCount[i];
        }

        static std::vector<int> currentOffsets;
        if (currentOffsets.size() != cellNum) currentOffsets.resize(cellNum);

        std::copy(cellStart.begin(), cellStart.end(), currentOffsets.begin());

        if (buffer.size() != particles.size()) buffer.resize(particles.size());

        for (const auto& p : particles) {
            int cx = (int)(p.x * invCellSize);
            int cy = (int)(p.y * invCellSize);

            if (cx < 0) cx = 0; else if (cx >= cols) cx = cols - 1;
            if (cy < 0) cy = 0; else if (cy >= rows) cy = rows - 1;

            int cellIdx = cx + cy * cols;
            int destIdx = currentOffsets[cellIdx]++;

            buffer[destIdx] = p;
        }

        std::swap(particles, buffer);
    }
};
