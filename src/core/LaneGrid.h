#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Config.h"
#include "render/Renderer.h"

// The lane system: hover highlight, click-to-toggle selection, lane drawing.
//
// For teammates:
//   game.lanes.isSelected(i)      -> is lane i currently selected?
//   game.lanes.selectedLanes()    -> list of selected lane indexes (B uses this for jump/duck)
//   game.lanes.hovered()          -> lane under the mouse, or -1 (D uses this for placement)
// Lane 0 = top lane, LANE_COUNT-1 = bottom lane (see Config.h).
class LaneGrid {
public:
    void reset();                                   // deselect everything
    void update(glm::vec2 mouse, bool leftClicked); // call every frame while PLAYING
    void clearHover() { hoveredLane = -1; }         // call while paused / in menus

    void drawLanes(Renderer& r) const;              // layer 2: the lane ground
    void drawHighlights(Renderer& r) const;         // layer 3: hover + selected overlay

    int  hovered() const { return hoveredLane; }
    bool isSelected(int lane) const;
    bool anySelected() const;
    std::vector<int> selectedLanes() const;

private:
    int  hoveredLane = -1;
    bool selected[LANE_COUNT] = {false};
};