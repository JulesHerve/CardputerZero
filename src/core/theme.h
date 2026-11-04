// =============================================================================
// theme.h  -  Runtime-selectable UI palettes (the three "skins")
//
// CardputerZero can wear three looks, chosen at runtime (and persisted to SD):
//
//   Nothing   - the original look: pure-black canvas, mono white glyphs, one red
//               accent. This is the DEFAULT and is kept pixel-for-pixel as it was.
//   Dell 1996 - an early-web catalogue: white page, black banners, sage ribbon
//               rows, a red call-to-action, blue "link" hints.
//   Nintendo  - brushed-metal periwinkle ground, carbon header/footer bars,
//               orange selection.
//
// Only COLOUR ROLES live here. Every drawing helper in ui.h / iconmenu.h reads
// the active theme through UI::cXxx(), so switching themes re-colours the whole
// UI without any app needing to change. A few *structural* differences (selection
// shape, hint styling) are branched on Theme::id inside ui.h - the style owns the
// pixels, not the individual page.
//
// Colours are the exact hex values from the design docs (docs/ui/styles/*.md),
// packed to RGB565 at compile time with the same formula M5GFX's color565() uses,
// so e.g. the Nothing palette is byte-identical to the old hard-coded values.
//
// FONTS: the mockups use web fonts (Doto, Space Mono, Arimo, Archivo Black,
// Azeret Mono, Silkscreen, Tinos) that are not on the device. Converting seven
// TTFs to bitmap fonts would add a lot of flash and cannot be verified without a
// hardware build, so every face is mapped to an M5GFX built-in: titles and body
// copy use fonts::Font2, hints/captions use fonts::Font0. See docs/ui notes.
// =============================================================================
#pragma once
#include <cstdint>

namespace core {

// Pack 8-8-8 RGB to RGB565 exactly as M5GFX::color565() does.
#define CZ565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

enum class ThemeId : uint8_t { Nothing = 0, Dell = 1, Nintendo = 2 };
constexpr int THEME_COUNT = 3;

struct Theme {
    ThemeId  id;
    uint16_t bg;       // page background
    uint16_t tile;     // icon chip / ribbon row body
    uint16_t bar;      // header & footer band
    uint16_t barText;  // text drawn ON the bar (bar can be dark or black)
    uint16_t text;     // body text
    uint16_t dim;      // secondary / muted text
    uint16_t line;     // hairlines & idle outlines
    uint16_t accent;   // selection / live-state / call-to-action
    uint16_t warn;     // transmit / send / caution
    uint16_t selBg;    // selected-row background
    uint16_t selFg;    // selected-row text
};

// --- Nothing OS (default) --- identical to the firmware's original palette ----
constexpr Theme THEME_NOTHING = {
    ThemeId::Nothing,
    CZ565(0x00, 0x00, 0x00),  // bg      true black
    CZ565(0x16, 0x16, 0x18),  // tile
    CZ565(0x0B, 0x0B, 0x0C),  // bar
    CZ565(0xF2, 0xF2, 0xF2),  // barText near-white
    CZ565(0xF2, 0xF2, 0xF2),  // text
    CZ565(0x6A, 0x6A, 0x70),  // dim
    CZ565(0x2A, 0x2A, 0x2E),  // line
    CZ565(0xD7, 0x17, 0x1E),  // accent  Nothing red
    CZ565(0xE6, 0xB4, 0x2C),  // warn    amber
    CZ565(0xD7, 0x17, 0x1E),  // selBg   red chip
    CZ565(0x00, 0x00, 0x00),  // selFg   black text on the chip
};

// --- Dell 1996 ----------------------------------------------------------------
// bar is pure black, so barText is white; selected row is a black bar with white
// text (the doc's "thick black" selection). accent = red CTA, warn = yellow.
constexpr Theme THEME_DELL = {
    ThemeId::Dell,
    CZ565(0xFF, 0xFF, 0xFF),  // bg      white page
    CZ565(0xB3, 0xBD, 0x95),  // tile    sage ribbon
    CZ565(0x00, 0x00, 0x00),  // bar     black banner
    CZ565(0xFF, 0xFF, 0xFF),  // barText white on black
    CZ565(0x00, 0x00, 0x00),  // text    black
    CZ565(0x5A, 0x5A, 0x5A),  // dim     grey
    CZ565(0x00, 0x00, 0x00),  // line    black rules
    CZ565(0xE9, 0x1D, 0x2A),  // accent  red CTA
    CZ565(0xFC, 0xC2, 0x0F),  // warn    yellow
    CZ565(0x00, 0x00, 0x00),  // selBg   black selection bar
    CZ565(0xFF, 0xFF, 0xFF),  // selFg   white text
};

// --- Nintendo 2001 ------------------------------------------------------------
// periwinkle metal ground, carbon bars (barText light), white selected row with
// dark text, orange accent, amber warn. dim (#3d4f97) is a LINE colour here -
// never used for body text on bg (handled in ui.h).
constexpr Theme THEME_NINTENDO = {
    ThemeId::Nintendo,
    CZ565(0x7A, 0x8A, 0xBA),  // bg      periwinkle metal
    CZ565(0xDE, 0xDE, 0xDE),  // tile    light tile
    CZ565(0x21, 0x24, 0x2E),  // bar     carbon
    CZ565(0xF2, 0xF2, 0xF2),  // barText light on carbon
    CZ565(0x21, 0x24, 0x2E),  // text    near-black
    CZ565(0x3D, 0x4F, 0x97),  // dim     indigo (line colour; used sparingly)
    CZ565(0x3D, 0x4F, 0x97),  // line    indigo
    CZ565(0xF6, 0x8D, 0x1F),  // accent  orange
    CZ565(0xEC, 0xAB, 0x37),  // warn    amber
    CZ565(0xFF, 0xFF, 0xFF),  // selBg   white selected row
    CZ565(0x21, 0x24, 0x2E),  // selFg   near-black text
};

// The live theme. Defaults to Nothing (optionally overridden at compile time with
// -DCZ_DEFAULT_THEME=1 for Dell or =2 for Nintendo).
extern Theme g_theme;

void        setTheme(ThemeId id);
ThemeId     currentTheme();
ThemeId     nextTheme();               // cycle Nothing -> Dell -> Nintendo -> ...
const char* themeName(ThemeId id);

}  // namespace core
