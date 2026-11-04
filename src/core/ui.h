// =============================================================================
// ui.h  -  Drawing helpers on top of M5GFX (M5Cardputer.Display)
//
// Three selectable visual styles share these helpers (see core/theme.h):
//   * Nothing OS (default) - pure-black canvas, mono white glyphs, one red accent
//   * Dell 1996            - white page, black banners, sage rows, blue hints
//   * Nintendo 2001        - periwinkle ground, carbon bars, orange selection
//
// Colours come from the live theme (g_theme) through the cXxx() accessors, so
// apps never touch raw colours. A few structural choices (selection shape, hint
// styling) branch on the active Theme::id here - the *style* owns those pixels,
// not the individual page. The Nothing look is byte-for-byte what it always was.
//
// Screen is 240x135, landscape.
// =============================================================================
#pragma once
#include <M5Cardputer.h>
#include <vector>
#include <string>
#include <cctype>
#include "theme.h"

namespace core {

class UI {
public:
    static constexpr int W = 240;
    static constexpr int H = 135;

    // --- colour roles (read the live theme) ---
    static uint16_t cAccent()  { return rgb(g_theme.accent); }
    static uint16_t cBg()      { return rgb(g_theme.bg); }
    static uint16_t cTile()    { return rgb(g_theme.tile); }
    static uint16_t cBar()     { return rgb(g_theme.bar); }
    static uint16_t cBarText() { return rgb(g_theme.barText); }
    static uint16_t cText()    { return rgb(g_theme.text); }
    static uint16_t cDim()     { return rgb(g_theme.dim); }
    static uint16_t cLine()    { return rgb(g_theme.line); }
    static uint16_t cWarn()    { return rgb(g_theme.warn); }
    static uint16_t cSelBg()   { return rgb(g_theme.selBg); }
    static uint16_t cSelFg()   { return rgb(g_theme.selFg); }

    static ThemeId  style()    { return g_theme.id; }

    static void clear() { M5Cardputer.Display.fillScreen(cBg()); }

    // The signature little red dot (Nothing only).
    static void redDot(int x, int y) { M5Cardputer.Display.fillCircle(x, y, 2, cAccent()); }

    // Title bar with an optional right-aligned hint (band, battery, etc.).
    static void header(const char* title, const char* right = nullptr) {
        auto& d = M5Cardputer.Display;
        d.fillRect(0, 0, W, 18, cBar());
        d.drawFastHLine(0, 18, W, cLine());
        if (style() == ThemeId::Nothing) {
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
        } else {
            // Dell & Nintendo: uppercase title on a dark bar; status in the
            // accent (Dell red / Nintendo orange). No signature dot.
            d.setTextDatum(middle_left);
            d.setTextColor(cBarText(), cBar());
            d.setFont(&fonts::Font2);
            d.drawString(upper(title), 8, 9);
            if (right) {
                d.setTextDatum(middle_right);
                d.setTextColor(cAccent(), cBar());
                d.setFont(&fonts::Font0);
                d.drawString(right, W - 8, 9);
            }
        }
    }

    static void footer(const char* hint) {
        auto& d = M5Cardputer.Display;
        d.fillRect(0, H - 14, W, 14, cBar());
        d.drawFastHLine(0, H - 15, W, cLine());
        d.setTextDatum(middle_left);
        d.setFont(&fonts::Font0);
        switch (style()) {
            case ThemeId::Dell: {
                // Blue underlined "link" hints on the black footer bar.
                uint16_t linkBlue = M5Cardputer.Display.color565(0x6E, 0xA8, 0xFF);
                d.setTextColor(linkBlue, cBar());
                d.drawString(hint, 8, H - 7);
                int w = d.textWidth(hint);
                d.drawFastHLine(8, H - 2, w, linkBlue);
                break;
            }
            case ThemeId::Nintendo:
                d.setTextColor(cWarn(), cBar());        // amber chips
                d.drawString(upper(hint), 8, H - 7);
                break;
            case ThemeId::Nothing:
            default:
                d.setTextColor(cDim(), cBar());
                d.drawString(hint, 8, H - 7);
                break;
        }
    }

    // Single-select scrolling list.
    static void list(const std::vector<std::string>& items, int sel, int top) {
        auto& d = M5Cardputer.Display;
        const int rowH = 16, y0 = 22, rows = 6;
        d.fillRect(0, 19, W, H - 19 - 14, cBg());
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        const bool rounded = (style() == ThemeId::Nothing);
        for (int i = 0; i < rows; ++i) {
            int idx = top + i;
            if (idx >= (int)items.size()) break;
            int y = y0 + i * rowH;
            bool on = (idx == sel);
            int textX = 12;
            if (on) {
                if (rounded) d.fillRoundRect(4, y - 1, W - 8, rowH, 4, cSelBg());
                else         d.fillRect(4, y - 1, W - 8, rowH, cSelBg());
            } else if (style() == ThemeId::Dell) {
                // square bullet before each unselected catalogue row
                d.fillRect(6, y + rowH / 2 - 3, 4, 4, cText());
                textX = 14;
            }
            d.setTextColor(on ? cSelFg() : cText(), on ? cSelBg() : cBg());
            d.drawString(items[idx].c_str(), textX, y + rowH / 2 - 1);
            if (on && style() == ThemeId::Nintendo) {       // orange arrow badge
                int ax = W - 14, ay = y + rowH / 2 - 1;
                d.fillTriangle(ax, ay - 4, ax, ay + 4, ax + 6, ay, cAccent());
            }
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

private:
    static uint16_t rgb(uint16_t packed565) { return packed565; }  // already RGB565

    // Uppercase into a rotating set of static buffers (Dell/Nintendo titles/hints).
    static const char* upper(const char* s) {
        static char buf[4][64];
        static int which = 0;
        which = (which + 1) & 3;
        char* o = buf[which];
        int i = 0;
        for (; s && s[i] && i < 63; ++i) o[i] = (char)toupper((unsigned char)s[i]);
        o[i] = 0;
        return o;
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
