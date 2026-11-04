// =============================================================================
// iconmenu.h  -  Reusable icon grid (home screen AND in-app sub-menus)
//
// Up to 6 items in a 3-column grid, rows centred horizontally. The selection
// treatment follows the active theme:
//   Nothing   - red ring + red caption
//   Dell      - sage ribbon card, thick black outline + red caption + NEW! tag
//   Nintendo  - light tile, orange ring + orange caption plate
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
            (void)d;
            drawTile(_items[i], cx, tileX, tileY, on);
        }
        UI::footer(n ? _items[_sel].subtext : "");
    }

private:
    static constexpr int COLS  = 3;
    static constexpr int CELLW = 74;
    static constexpr int CELLH = 48;
    std::vector<IconItem> _items;
    int _sel = 0;

    static void drawTile(const IconItem& it, int cx, int tileX, int tileY, bool on) {
        auto& d = M5Cardputer.Display;
        switch (UI::style()) {

        case ThemeId::Dell: {
            d.fillRect(tileX, tileY, 38, 38, UI::cTile());          // sage ribbon body
            if (on) {                                               // thick black outline
                for (int k = 0; k < 3; ++k) d.drawRect(tileX - k, tileY - k, 38 + 2 * k, 38 + 2 * k, UI::cLine());
            } else {
                d.drawRect(tileX, tileY, 38, 38, UI::cLine());
            }
            drawIcon(it.icon, cx, tileY + 18, UI::cText());         // black glyph on sage
            if (on) {                                               // yellow NEW! sticker
                d.fillRect(tileX + 24, tileY - 5, 20, 10, UI::cWarn());
                d.drawRect(tileX + 24, tileY - 5, 20, 10, UI::cLine());
                d.setFont(&fonts::Font0); d.setTextDatum(middle_center);
                d.setTextColor(UI::cLine(), UI::cWarn());
                d.drawString("NEW", tileX + 34, tileY);
            }
            d.setFont(&fonts::Font0); d.setTextDatum(middle_center);
            d.setTextColor(on ? UI::cAccent() : UI::cText(), UI::cBg());
            d.drawString(it.label, cx, tileY + 44);
            break;
        }

        case ThemeId::Nintendo: {
            d.fillRoundRect(tileX, tileY, 38, 38, 8, UI::cTile());  // light metal tile
            if (on) {
                d.drawRoundRect(tileX, tileY, 38, 38, 8, UI::cAccent());
                d.drawRoundRect(tileX + 1, tileY + 1, 36, 36, 7, UI::cAccent());
            } else {
                d.drawRoundRect(tileX, tileY, 38, 38, 8, UI::cLine());
            }
            drawIcon(it.icon, cx, tileY + 18, UI::cText());         // dark glyph on light tile
            if (on) {                                               // orange caption plate
                int pw = 46;
                d.fillRect(cx - pw / 2, tileY + 40, pw, 11, UI::cAccent());
                d.setFont(&fonts::Font0); d.setTextDatum(middle_center);
                d.setTextColor(UI::cSelFg(), UI::cAccent());
                d.drawString(it.label, cx, tileY + 45);
            } else {
                d.setFont(&fonts::Font0); d.setTextDatum(middle_center);
                d.setTextColor(UI::cText(), UI::cBg());
                d.drawString(it.label, cx, tileY + 45);
            }
            break;
        }

        case ThemeId::Nothing:
        default: {                                                  // original look, unchanged
            d.fillRoundRect(tileX, tileY, 38, 38, 9, UI::cTile());
            if (on) {
                d.drawRoundRect(tileX, tileY, 38, 38, 9, UI::cAccent());
                d.drawRoundRect(tileX + 1, tileY + 1, 36, 36, 8, UI::cAccent());
            }
            drawIcon(it.icon, cx, tileY + 19, on ? UI::cText() : UI::cDim());
            d.setFont(&fonts::Font0);
            d.setTextDatum(middle_center);
            d.setTextColor(on ? UI::cAccent() : UI::cDim(), UI::cBg());
            d.drawString(it.label, cx, tileY + 44);
            break;
        }
        }
    }
};

}  // namespace core
