#include "LaneGrid.h"

void LaneGrid::reset() {
    hoveredLane = -1;
    for (bool& s : selected) s = false;
}

void LaneGrid::update(glm::vec2 mouse, bool leftClicked) {
    hoveredLane = -1;
    // only inside the playfield width; the UI strip and black bars give -1
    if (mouse.x >= 0.0f && mouse.x <= VIRTUAL_W)
        hoveredLane = laneFromY(mouse.y);

    if (leftClicked && hoveredLane >= 0)
        selected[hoveredLane] = !selected[hoveredLane];   // click toggles
}

bool LaneGrid::isSelected(int lane) const {
    return lane >= 0 && lane < LANE_COUNT && selected[lane];
}

bool LaneGrid::anySelected() const {
    for (bool s : selected) if (s) return true;
    return false;
}

std::vector<int> LaneGrid::selectedLanes() const {
    std::vector<int> out;
    for (int i = 0; i < LANE_COUNT; i++) if (selected[i]) out.push_back(i);
    return out;
}

void LaneGrid::drawLanes(Renderer& r) const {
    // ---- Palette (Egyptian sandstone). Change the theme colours here only. ----
    const glm::vec4 sandLight(0.87f, 0.75f, 0.52f, 1.0f);
    const glm::vec4 sandDark (0.80f, 0.67f, 0.44f, 1.0f);
    const glm::vec4 homeStone(0.55f, 0.46f, 0.34f, 1.0f);
    const glm::vec4 line     (0.35f, 0.26f, 0.15f, 1.0f);

    for (int i = 0; i < LANE_COUNT; i++) {
        float y = laneBottomY(i);

        // strip under the home base (the home base model is drawn on top of this later)
        r.rectBL({0.0f, y}, {HOME_W, LANE_H}, homeStone);

        // checkered sand tiles across the field
        for (int c = 0; c < GRID_COLS; c++) {
            const glm::vec4& col = ((c + i) % 2 == 0) ? sandLight : sandDark;
            r.rectBL({FIELD_X0 + c * CELL_W, y}, {CELL_W, LANE_H}, col);
        }
    }

    // faint vertical lines between tiles
    for (int c = 1; c < GRID_COLS; c++)
        r.rectBL({FIELD_X0 + c * CELL_W - 1.0f, 0.0f}, {2.0f, LANE_COUNT * LANE_H},
                 {line.r, line.g, line.b, 0.18f});

    // dark lines between lanes (and on the outer edges)
    for (int i = 0; i <= LANE_COUNT; i++)
        r.rect({VIRTUAL_W * 0.5f, i * LANE_H}, {VIRTUAL_W, 4.0f}, line);
    r.rectBL({VIRTUAL_W - 4.0f, 0.0f}, {4.0f, LANE_COUNT * LANE_H}, line);
}

void LaneGrid::drawHighlights(Renderer& r) const {
    for (int i = 0; i < LANE_COUNT; i++) {
        float bottom = laneBottomY(i);
        if (selected[i]) {
            // locked highlight: warm tint + bright border so it can't be missed
            r.rectBL({0.0f, bottom}, {VIRTUAL_W, LANE_H}, {1.0f, 0.95f, 0.45f, 0.30f});
            r.rect({VIRTUAL_W * 0.5f, bottom + 3.0f},          {VIRTUAL_W, 6.0f}, {1.0f, 0.9f, 0.2f, 0.9f});
            r.rect({VIRTUAL_W * 0.5f, bottom + LANE_H - 3.0f}, {VIRTUAL_W, 6.0f}, {1.0f, 0.9f, 0.2f, 0.9f});
        } else if (i == hoveredLane) {
            r.rectBL({0.0f, bottom}, {VIRTUAL_W, LANE_H}, {1.0f, 1.0f, 1.0f, 0.20f});
        }
    }
}