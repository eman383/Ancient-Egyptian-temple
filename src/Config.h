#pragma once
// Shared constants. Everyone on the team uses these.
// Coordinates: virtual 1600x900, origin BOTTOM-LEFT, +x right, +y up.

constexpr float VIRTUAL_W = 1600.0f;
constexpr float VIRTUAL_H = 900.0f;

constexpr float UI_H       = 150.0f;   // top strip: cards + coin counter
constexpr int   LANE_COUNT = 5;
constexpr float LANE_H     = 150.0f;   // 5 * 150 = 750
constexpr float HOME_W     = 200.0f;   // home base occupies x in [0, 200]
constexpr float FIELD_X0   = HOME_W;   // players start here
constexpr float FIELD_X1   = VIRTUAL_W;// enemies spawn here

// Visual grid inside each lane (placement is per lane, columns are for looks and spacing)
constexpr int   GRID_COLS  = 10;
constexpr float CELL_W     = (VIRTUAL_W - HOME_W) / GRID_COLS;   // 140

// Lane 0 is the TOP lane (just under the UI), lane 4 the bottom one.
inline float laneBottomY(int lane) { return (LANE_COUNT - 1 - lane) * LANE_H; }
inline float laneCenterY(int lane) { return laneBottomY(lane) + LANE_H * 0.5f; }
// Returns -1 if y is not inside the lane area (e.g. over the UI strip).
inline int laneFromY(float y) {
    if (y < 0.0f || y >= LANE_COUNT * LANE_H) return -1;
    return LANE_COUNT - 1 - static_cast<int>(y / LANE_H);
}