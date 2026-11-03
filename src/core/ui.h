// =============================================================================
// ui.h  -  Drawing helpers on top of M5GFX (M5Cardputer.Display)
//
// Visual language: "Nothing OS" inspired — pure-black canvas, monochrome white
// glyphs, a single signature red accent, generous spacing, uppercase letter-
// spaced labels. The home screen (in main.cpp) renders an iOS-style grid of
// rounded app icons with a label under each; these helpers style the in-app
// screens (title bar, lists, banners, console) to match.
//
// Screen is 240x135, landscape.
// =============================================================================
#pragma once
#include <M5Cardputer.h>
#include <vector>
#include <string>

namespace core {

class UI {
public:
    static constexpr int W = 240;
    static constexpr int H = 135;

    // --- Nothing-OS palette (RGB565) ---
    static uint16_t cAccent() { return M5Cardputer.Display.color565(0xD7, 0x17, 0x1E); } // Nothing red
    static uint16_t cBg()     { return M5Cardputer.Display.color565(0x00, 0x00, 0x00); } // true black
    static uint16_t cTile()   { return M5Cardputer.Display.color565(0x16, 0x16, 0x18); } // icon chip
    static uint16_t cBar()    { return M5Cardputer.Display.color565(0x0B, 0x0B, 0x0C); } // bars
    static uint16_t cText()   { return M5Cardputer.Display.color565(0xF2, 0xF2, 0xF2); } // near-white
    static uint16_t cDim()    { return M5Cardputer.Display.color565(0x6A, 0x6A, 0x70); } // grey
    static uint16_t cLine()   { return M5Cardputer.Display.color565(0x2A, 0x2A, 0x2E); } // hairline
    static uint16_t cWarn()   { return M5Cardputer.Display.color565(0xE6, 0xB4, 0x2C); } // amber

    static void clear() { M5Cardputer.Display.fillScreen(cBg()); }

    // The signature little red dot.
    static void redDot(int x, int y) { M5Cardputer.Display.fillCircle(x, y, 2, cAccent()); }

    // Title bar with an optional right-aligned hint (band, battery, etc.).
    static void header(const char* title, const char* right = nullptr) {
        auto& d = M5Cardputer.Display;
        d.fillRect(0, 0, W, 18, cBar());
        d.drawFastHLine(0, 18, W, cLine());
        d.setTextDatum(middle_left);
        d.setTextColor(cText(), cBar());
        d.setFont(&fonts::Font2);
        d.drawString(title, 8, 9);
        if (right) {
            d.setTextDatum(middle_right);
            d.setTextColor(cDim(), cBar());
            d.setFont(&fonts::Font0);
            d.drawString(right, W - 14, 9);
        }
        redDot(W - 7, 9);
    }

    static void footer(const char* hint) {
        auto& d = M5Cardputer.Display;
        d.fillRect(0, H - 14, W, 14, cBar());
        d.drawFastHLine(0, H - 15, W, cLine());
        d.setTextDatum(middle_left);
        d.setTextColor(cDim(), cBar());
        d.setFont(&fonts::Font0);
        d.drawString(hint, 8, H - 7);
    }

    // Single-select scrolling list. Selected row = red chip with black text.
    static void list(const std::vector<std::string>& items, int sel, int top) {
        auto& d = M5Cardputer.Display;
        const int rowH = 16, y0 = 22, rows = 6;
        d.fillRect(0, 19, W, H - 19 - 14, cBg());
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        for (int i = 0; i < rows; ++i) {
            int idx = top + i;
            if (idx >= (int)items.size()) break;
            int y = y0 + i * rowH;
            bool on = (idx == sel);
            if (on) d.fillRoundRect(4, y - 1, W - 8, rowH, 4, cAccent());
            d.setTextColor(on ? cBg() : cText(), on ? cAccent() : cBg());
            d.drawString(items[idx].c_str(), 12, y + rowH / 2 - 1);
        }
    }

    // Centred message (supports a single '\n' via M5GFX multiline drawString).
    static void banner(const char* msg, uint16_t fg) {
        auto& d = M5Cardputer.Display;
        d.fillRect(0, 19, W, H - 19 - 14, cBg());
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_center);
        d.setTextColor(fg, cBg());
        d.drawString(msg, W / 2, H / 2);
    }

    static void move(int delta, int count, int& sel, int& top, int rows = 6) {
        if (count == 0) return;
        sel = (sel + delta + count) % count;
        if (sel < top) top = sel;
        if (sel >= top + rows) top = sel - rows + 1;
    }
};

// Scrolling text console for the radio / scanner apps.
class Console {
public:
    void reset() { _lines.clear(); }
    void line(const std::string& s) {
        _lines.push_back(s);
        if (_lines.size() > 64) _lines.erase(_lines.begin());
        draw();
    }
    void draw() {
        auto& d = M5Cardputer.Display;
        const int y0 = 22, rowH = 12, rows = (UI::H - 22 - 14) / rowH;
        d.fillRect(0, 19, UI::W, UI::H - 19 - 14, UI::cBg());
        d.setFont(&fonts::Font0);
        d.setTextDatum(top_left);
        d.setTextColor(UI::cText(), UI::cBg());
        int start = (int)_lines.size() > rows ? _lines.size() - rows : 0;
        for (int i = start; i < (int)_lines.size(); ++i)
            d.drawString(_lines[i].c_str(), 6, y0 + (i - start) * rowH);
    }
private:
    std::vector<std::string> _lines;
};

}  // namespace core
