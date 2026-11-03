// =============================================================================
// iconmenu.h  -  Reusable iOS-style icon grid (used by the home screen AND the
//                in-app sub-menus, so every menu is icons, not a list).
//
// Up to 6 items in a 3-column grid; rows are centred horizontally. The selected
// tile gets the Nothing-red ring and red label, and the footer shows that
// item's one-line subtext.
// =============================================================================
#pragma once
#include "ui.h"
#include "icons.h"
#include "input.h"
#include <vector>

namespace core {

struct IconItem {
    Icon        icon;
    const char* label;
    const char* subtext;
};

class IconMenu {
public:
    void clear() { _items.clear(); _sel = 0; }
    void add(Icon ic, const char* label, const char* sub) { _items.push_back({ic, label, sub}); }
    int  selection() const { return _sel; }
    int  count() const { return (int)_items.size(); }

    // Grid move with wrap. dCol/dRow in {-1,0,+1}.
    void move(int dCol, int dRow) {
        int n = count(); if (!n) return;
        int rows = (n + COLS - 1) / COLS;
        int col = _sel % COLS, row = _sel / COLS;
        if (dCol) col = (col + dCol + COLS) % COLS;
        if (dRow) row = (row + dRow + rows) % rows;
        int idx = row * COLS + col;
        if (idx >= n) idx = n - 1;      // snap into the last (short) row
        _sel = idx;
    }

    void draw(const char* title, const char* right = nullptr) {
        auto& d = M5Cardputer.Display;
        UI::clear();
        UI::header(title, right);
        int n = count();
        int rows = (n + COLS - 1) / COLS;
        int topY = (rows == 1) ? 44 : 22;
        for (int i = 0; i < n; ++i) {
            int row = i / COLS;
            int inRow = (row == rows - 1) ? (n - row * COLS) : COLS;
            int startX = (UI::W - inRow * CELLW) / 2;
            int col = i % COLS;
            int cx = startX + col * CELLW + CELLW / 2;
            int tileY = topY + row * CELLH;
            int tileX = cx - 19;
            bool on = (i == _sel);
            d.fillRoundRect(tileX, tileY, 38, 38, 9, UI::cTile());
            if (on) {
                d.drawRoundRect(tileX, tileY, 38, 38, 9, UI::cAccent());
                d.drawRoundRect(tileX + 1, tileY + 1, 36, 36, 8, UI::cAccent());
            }
            drawIcon(_items[i].icon, cx, tileY + 19, on ? UI::cText() : UI::cDim());
            d.setFont(&fonts::Font0);
            d.setTextDatum(middle_center);
            d.setTextColor(on ? UI::cAccent() : UI::cDim(), UI::cBg());
            d.drawString(_items[i].label, cx, tileY + 44);
        }
        UI::footer(n ? _items[_sel].subtext : "");
    }

private:
    static constexpr int COLS  = 3;
    static constexpr int CELLW = 74;
    static constexpr int CELLH = 48;
    std::vector<IconItem> _items;
    int _sel = 0;
};

}  // namespace core
