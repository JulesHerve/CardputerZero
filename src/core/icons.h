// =============================================================================
// icons.h  -  Shared monochrome line-art glyphs (Nothing-OS style)
//
// Every icon is drawn centred on (cx, cy) inside a ~38px rounded tile using
// only primitives confirmed present in the installed M5GFX (drawLine,
// drawCircle, fillCircle, drawRect, fillRect, drawFastH/VLine). Arcs are
// avoided so orientation never depends on the library's angle convention.
// =============================================================================
#pragma once
#include <M5Cardputer.h>
#include <math.h>

namespace core {

enum class Icon {
    // home / apps
    SubGhz, Nfc, Ir, Wifi, Ble, Usb,
    // actions
    Rx, Tx, Replay, Analyze, Freq, Load, Save,
    Learn, Power, Read, Dump, Scan, Keyboard, Info,
};

inline void drawIcon(Icon g, int cx, int cy, uint16_t c) {
    auto& d = M5Cardputer.Display;
    switch (g) {
    case Icon::SubGhz: {                 // RF waveform
        int px = cx - 15, py = cy;
        for (int i = 1; i <= 6; ++i) {
            int x = cx - 15 + i * 5, y = cy + ((i % 2) ? -8 : 8);
            d.drawLine(px, py, x, y, c); px = x; py = y;
        }
        d.drawLine(px, py, cx + 15, cy, c);
        break;
    }
    case Icon::Nfc:                      // contactless chevrons + emitter
        d.fillCircle(cx - 12, cy, 2, c);
        for (int k = 0; k < 3; ++k) {
            int x = cx - 6 + k * 7, h = 6 + k * 3, dp = 5 + k * 2;
            d.drawLine(x, cy - h, x + dp, cy, c);
            d.drawLine(x + dp, cy, x, cy + h, c);
        }
        break;
    case Icon::Ir: {                     // emitter dot + beam dashes
        d.fillCircle(cx - 12, cy + 10, 3, c);
        for (int i = 0; i < 3; ++i) { int o = i * 7; d.drawLine(cx - 6 + o, cy + 6, cx - 1 + o, cy - 2 - o / 2, c); }
        d.drawCircle(cx + 12, cy - 10, 3, c);
        break;
    }
    case Icon::Wifi:                     // stacked chevrons + base dot
        d.fillCircle(cx, cy + 12, 2, c);
        for (int k = 0; k < 3; ++k) {
            int y = cy + 6 - k * 6, w = 6 + k * 5, dp = 5 + k * 3;
            d.drawLine(cx - w, y, cx, y - dp, c);
            d.drawLine(cx, y - dp, cx + w, y, c);
        }
        break;
    case Icon::Ble: {                    // Bluetooth rune
        int t = cy - 14, b = cy + 14, m = cy, r = cx + 8, l = cx - 8;
        d.drawLine(cx, t, cx, b, c);
        d.drawLine(cx, t, r, cy - 7, c); d.drawLine(r, cy - 7, cx, m, c);
        d.drawLine(cx, b, r, cy + 7, c); d.drawLine(r, cy + 7, cx, m, c);
        d.drawLine(l, cy - 7, r, cy + 7, c); d.drawLine(l, cy + 7, r, cy - 7, c);
        break;
    }
    case Icon::Usb:                      // USB trident
        d.drawLine(cx, cy + 15, cx, cy - 13, c);
        d.fillCircle(cx, cy + 15, 3, c); d.fillCircle(cx, cy - 15, 2, c);
        d.drawLine(cx, cy - 2, cx - 9, cy - 9, c); d.fillCircle(cx - 9, cy - 11, 3, c);
        d.drawLine(cx, cy - 6, cx + 9, cy - 13, c); d.fillRect(cx + 7, cy - 17, 5, 5, c);
        break;

    case Icon::Rx:                       // down arrow into a tray
        d.drawLine(cx, cy - 13, cx, cy + 6, c);
        d.drawLine(cx, cy + 6, cx - 6, cy, c); d.drawLine(cx, cy + 6, cx + 6, cy, c);
        d.drawLine(cx - 12, cy + 12, cx + 12, cy + 12, c);
        d.drawLine(cx - 12, cy + 12, cx - 12, cy + 7, c); d.drawLine(cx + 12, cy + 12, cx + 12, cy + 7, c);
        break;
    case Icon::Tx:                       // up arrow out of a tray
        d.drawLine(cx, cy + 13, cx, cy - 6, c);
        d.drawLine(cx, cy - 6, cx - 6, cy, c); d.drawLine(cx, cy - 6, cx + 6, cy, c);
        d.drawLine(cx - 12, cy + 12, cx + 12, cy + 12, c);
        d.drawLine(cx - 12, cy + 12, cx - 12, cy + 7, c); d.drawLine(cx + 12, cy + 12, cx + 12, cy + 7, c);
        break;
    case Icon::Replay:                   // loop ring + arrowhead
        d.drawCircle(cx, cy, 12, c); d.drawCircle(cx, cy, 11, c);
        d.drawLine(cx + 12, cy - 4, cx + 12, cy + 4, c);
        d.drawLine(cx + 12, cy - 4, cx + 7, cy - 2, c); d.drawLine(cx + 12, cy - 4, cx + 16, cy - 1, c);
        break;
    case Icon::Analyze:                  // spectrum bars
        for (int i = 0; i < 4; ++i) {
            int h = 6 + ((i == 1) ? 14 : (i == 2) ? 9 : i * 3);
            int x = cx - 12 + i * 8;
            d.fillRect(x, cy + 12 - h, 4, h, c);
        }
        break;
    case Icon::Freq: {                   // tuning sine + marker
        int px = cx - 14, py = cy;
        for (int i = 1; i <= 14; ++i) {
            int x = cx - 14 + i * 2;
            int y = cy - (int)(8 * sinf(i * 0.9f));
            d.drawLine(px, py, x, y, c); px = x; py = y;
        }
        d.drawLine(cx, cy - 15, cx, cy + 15, c);
        break;
    }
    case Icon::Load:                     // folder
        d.drawRect(cx - 13, cy - 6, 26, 16, c);
        d.drawLine(cx - 13, cy - 6, cx - 6, cy - 6, c);
        d.drawLine(cx - 6, cy - 6, cx - 3, cy - 10, c); d.drawLine(cx - 3, cy - 10, cx + 10, cy - 10, c);
        d.drawLine(cx + 10, cy - 10, cx + 10, cy - 6, c);
        break;
    case Icon::Save:                     // down arrow onto a disk line
        d.drawLine(cx, cy - 12, cx, cy + 4, c);
        d.drawLine(cx, cy + 4, cx - 5, cy - 1, c); d.drawLine(cx, cy + 4, cx + 5, cy - 1, c);
        d.drawLine(cx - 12, cy + 11, cx + 12, cy + 11, c);
        break;
    case Icon::Learn:                    // capture target
        d.drawCircle(cx, cy, 12, c); d.fillCircle(cx, cy, 3, c);
        d.drawLine(cx - 16, cy, cx - 10, cy, c); d.drawLine(cx + 10, cy, cx + 16, cy, c);
        d.drawLine(cx, cy - 16, cx, cy - 10, c); d.drawLine(cx, cy + 10, cx, cy + 16, c);
        break;
    case Icon::Power:                    // power symbol: ring + vertical bar
        d.drawCircle(cx, cy + 2, 11, c); d.drawCircle(cx, cy + 2, 10, c);
        d.fillRect(cx - 2, cy - 14, 4, 4, c);         // mask the ring top with bar
        d.drawLine(cx, cy - 13, cx, cy + 1, c);
        d.drawLine(cx - 1, cy - 13, cx - 1, cy + 1, c);
        break;
    case Icon::Read:                     // magnifier
        d.drawCircle(cx - 3, cy - 3, 9, c);
        d.drawLine(cx + 4, cy + 4, cx + 12, cy + 12, c);
        d.drawLine(cx + 5, cy + 3, cx + 13, cy + 11, c);
        break;
    case Icon::Dump:                     // document with lines
        d.drawRect(cx - 10, cy - 14, 20, 28, c);
        for (int i = 0; i < 4; ++i) d.drawLine(cx - 6, cy - 8 + i * 6, cx + 6, cy - 8 + i * 6, c);
        break;
    case Icon::Scan:                     // radar rings + sweep
        d.drawCircle(cx, cy, 4, c); d.drawCircle(cx, cy, 9, c); d.drawCircle(cx, cy, 14, c);
        d.drawLine(cx, cy, cx + 12, cy - 9, c);
        break;
    case Icon::Keyboard: {               // key grid
        d.drawRect(cx - 15, cy - 9, 30, 18, c);
        for (int r = 0; r < 2; ++r)
            for (int k = 0; k < 4; ++k)
                d.drawRect(cx - 12 + k * 7, cy - 6 + r * 7, 4, 4, c);
        break;
    }
    case Icon::Info:                     // info 'i'
        d.drawCircle(cx, cy, 13, c);
        d.fillCircle(cx, cy - 6, 2, c);
        d.drawLine(cx, cy - 1, cx, cy + 7, c);
        break;
    }
}

}  // namespace core
