#include "ui/Cards.h"

#include <algorithm>
#include <string>

using namespace cards;

namespace {
const glm::vec4 GOLD      {0.85f, 0.62f, 0.12f, 1.0f};
const glm::vec4 GOLD_DARK {0.55f, 0.36f, 0.05f, 1.0f};
const glm::vec4 PANEL     {0.93f, 0.82f, 0.58f, 1.0f};
const glm::vec4 PANEL_HOT {1.00f, 0.94f, 0.74f, 1.0f};
const glm::vec4 INK       {0.22f, 0.14f, 0.06f, 1.0f};

// ---- tiny 7-segment number renderer (the engine has no text) ----
//   segments: a=top b=top-right c=bottom-right d=bottom e=bottom-left f=top-left g=middle
constexpr int DIGIT_MASK[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

float digitW(float h)   { return h * 0.55f; }
float digitGap(float h) { return h * 0.20f; }

float numberWidth(int value, float h) {
    const int n = static_cast<int>(std::to_string(std::max(value, 0)).size());
    return n * digitW(h) + (n - 1) * digitGap(h);
}

void drawDigit(Renderer& r, int digit, float cx, float cy, float h, const glm::vec4& col) {
    const float w = digitW(h), t = h * 0.15f;
    const int mask = DIGIT_MASK[digit];
    if (mask & 1)  r.rect({cx, cy + h * 0.5f - t * 0.5f}, {w, t}, col);                      // a
    if (mask & 64) r.rect({cx, cy}, {w, t}, col);                                            // g
    if (mask & 8)  r.rect({cx, cy - h * 0.5f + t * 0.5f}, {w, t}, col);                      // d
    if (mask & 2)  r.rect({cx + w * 0.5f - t * 0.5f, cy + h * 0.25f}, {t, h * 0.5f}, col);   // b
    if (mask & 4)  r.rect({cx + w * 0.5f - t * 0.5f, cy - h * 0.25f}, {t, h * 0.5f}, col);   // c
    if (mask & 16) r.rect({cx - w * 0.5f + t * 0.5f, cy - h * 0.25f}, {t, h * 0.5f}, col);   // e
    if (mask & 32) r.rect({cx - w * 0.5f + t * 0.5f, cy + h * 0.25f}, {t, h * 0.5f}, col);   // f
}

// Draws the number with its LEFT edge at x, vertically centred on cy.
void drawNumber(Renderer& r, int value, float x, float cy, float h, const glm::vec4& col) {
    const std::string s = std::to_string(std::max(value, 0));
    float cx = x + digitW(h) * 0.5f;
    for (char c : s) {
        drawDigit(r, c - '0', cx, cy, h, col);
        cx += digitW(h) + digitGap(h);
    }
}

void drawCoin(Renderer& r, glm::vec2 c, float radius) {
    r.circle(c, radius, GOLD_DARK);
    r.circle(c, radius * 0.86f, {1.0f, 0.82f, 0.25f, 1.0f});
    r.circle(c, radius * 0.62f, {0.95f, 0.70f, 0.12f, 1.0f});
    r.rect(c, {radius * 0.22f, radius * 0.80f}, {1.0f, 0.90f, 0.45f, 1.0f});   // embossed bar
}

void drawLock(Renderer& r, glm::vec2 c) {
    r.circle(c + glm::vec2(0.0f, 8.0f), 8.0f, INK);                       // shackle
    r.circle(c + glm::vec2(0.0f, 8.0f), 4.5f, PANEL);
    r.rect(c + glm::vec2(0.0f, -2.0f), {22.0f, 16.0f}, INK);              // body
    r.circle(c + glm::vec2(0.0f, -1.0f), 2.5f, PANEL);                    // keyhole
    r.rect(c + glm::vec2(0.0f, -5.0f), {2.5f, 6.0f}, PANEL);
}
}

CardsUI::CardsUI()
    : melee_(iconFeet(0), ICON_H),
      defense_(iconFeet(1), ICON_H),
      longRange_(iconFeet(2), ICON_H)
{
}

PlayerKind CardsUI::kindOf(int card)
{
    switch (card) {
        case 0:  return PlayerKind::Melee;
        case 1:  return PlayerKind::Defense;
        default: return PlayerKind::LongRange;
    }
}

bool CardsUI::isUnlocked(int card) const
{
    return economy_ && economy_->canAfford(cost(kindOf(card)));
}

void CardsUI::setup(Game& game, PlayerManager& players, Economy& economy)
{
    players_ = &players;
    economy_ = &economy;
    economy_->coins = STARTING_COINS;

    game.onGameplayUpdate([this, &game](float dt) { update(game, dt); });
    game.onDraw(Layer::Hud, [this](Renderer& r) { draw(r); });
    game.onReset([this] {
        economy_->coins = STARTING_COINS;
        hovered_ = -1;
        std::fill(std::begin(flash_), std::end(flash_), 0.0f);
    });
}

void CardsUI::update(Game& game, float dt)
{
    for (float& f : flash_) f = std::max(0.0f, f - dt);

    // Only UNLOCKED cards react to the mouse. Locked cards are never hovered or clicked.
    hovered_ = -1;
    const glm::vec2 m = game.input.mouse;
    for (int i = 0; i < COUNT; ++i) {
        if (!isUnlocked(i)) continue;
        const float x = left(i);
        if (m.x >= x && m.x <= x + W && m.y >= Y && m.y <= Y + H) hovered_ = i;
    }

    if (hovered_ >= 0 && game.input.leftClicked()) onCardClicked(game, hovered_);
}

void CardsUI::onCardClicked(Game& game, int card)
{
    const PlayerKind kind = kindOf(card);
    if (!economy_->canAfford(cost(kind))) return;

    players_->selectType(kind);
    if (players_->placeOnSelectedLane(game)) economy_->spend(cost(kind));
    else flash_[card] = 0.45f;   // no lane / several lanes selected: nothing placed
}

void CardsUI::draw(Renderer& r) const
{
    for (int i = 0; i < COUNT; ++i) drawCard(r, i);
    drawCoinCounter(r);
}

void CardsUI::drawCard(Renderer& r, int card) const
{
    const bool unlocked = isUnlocked(card);
    const bool hot      = unlocked && hovered_ == card;
    const bool selected = unlocked && players_ && players_->selectedType() == kindOf(card);
    const bool refused  = flash_[card] > 0.0f;

    const float x  = left(card);
    const glm::vec2 centre(x + W * 0.5f, Y + H * 0.5f);

    r.setGrayscale(!unlocked);     // locked cards are drawn in grayscale values only

    // frame
    glm::vec4 frame = GOLD;
    if (selected) frame = {1.0f, 1.0f, 0.55f, 1.0f};
    if (hot)      frame = {1.0f, 1.0f, 1.0f, 1.0f};
    if (refused)  frame = {0.95f, 0.15f, 0.15f, 1.0f};
    const float pad = (hot || selected || refused) ? 5.0f : 3.0f;
    r.rect(centre, {W + 2 * pad, H + 2 * pad}, frame);
    r.rect(centre, {W, H}, hot ? PANEL_HOT : PANEL);

    // player model
    switch (card) {
        case 0:  melee_.draw(r);     break;
        case 1:  defense_.draw(r);   break;
        default: longRange_.draw(r); break;
    }

    // cost row: coin + price
    const int price = cost(kindOf(card));
    const float rowY = Y + 17.0f;
    const float numH = 22.0f;
    const float total = 2 * 11.0f + 8.0f + numberWidth(price, numH);
    const float startX = centre.x - total * 0.5f;
    drawCoin(r, {startX + 11.0f, rowY}, 11.0f);
    drawNumber(r, price, startX + 22.0f + 8.0f, rowY, numH, INK);

    if (!unlocked) drawLock(r, {x + W - 20.0f, Y + H - 24.0f});

    r.setGrayscale(false);
}

void CardsUI::drawCoinCounter(Renderer& r) const
{
    const glm::vec2 c(940.0f, 828.0f);
    r.rect(c, {268.0f, 98.0f}, GOLD);
    r.rect(c, {258.0f, 88.0f}, {0.30f, 0.20f, 0.10f, 1.0f});
    drawCoin(r, {c.x - 98.0f, c.y}, 30.0f);

    const int coins = economy_ ? economy_->coins : 0;
    drawNumber(r, coins, c.x - 55.0f, c.y, 56.0f, {1.0f, 0.92f, 0.55f, 1.0f});
}
