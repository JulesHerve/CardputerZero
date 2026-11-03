// =============================================================================
// wifi_scan.cpp  -  implementation of the Wi-Fi tool (passive)
// =============================================================================
#include "wifi_scan.h"
#include <WiFi.h>
#include <vector>
#include <string>

using namespace core;

namespace {
const char* encStr(wifi_auth_mode_t m) {
    switch (m) {
        case WIFI_AUTH_OPEN:         return "OPEN";
        case WIFI_AUTH_WEP:          return "WEP";
        case WIFI_AUTH_WPA_PSK:      return "WPA";
        case WIFI_AUTH_WPA2_PSK:     return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
        case WIFI_AUTH_WPA3_PSK:     return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:return "WPA2/3";
        default:                     return "?";
    }
}
}  // namespace

namespace apps {

void WifiScan::onEnter() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
    _view = View::Menu;
    if (!_menuBuilt) {
        _menu.add(core::Icon::Scan,    "SCAN",     "Nearby APs, strongest first");
        _menu.add(core::Icon::Analyze, "CHANNELS", "AP count per 2.4 GHz channel");
        _menu.add(core::Icon::Info,    "DETAIL",   "Full info for one AP");
        _menuBuilt = true;
    }
    _menu.draw("WiFi");
}

void WifiScan::onExit() {
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
}

void WifiScan::scan() {
    UI::clear(); UI::header("WiFi"); UI::banner("Scanning...", UI::cAccent());
    _count = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
    if (_count < 0) _count = 0;
    if (_count > 64) _count = 64;
    // selection-sort indices by RSSI, strongest first
    for (int i = 0; i < _count; ++i) _order[i] = i;
    for (int i = 0; i < _count; ++i)
        for (int j = i + 1; j < _count; ++j)
            if (WiFi.RSSI(_order[j]) > WiFi.RSSI(_order[i])) {
                int t = _order[i]; _order[i] = _order[j]; _order[j] = t;
            }
    _sel = _top = 0;
}

void WifiScan::drawList() {
    UI::clear();
    char hdr[20]; snprintf(hdr, sizeof(hdr), "%d APs", _count);
    UI::header("WiFi scan", hdr);
    if (_count == 0) { UI::banner("No networks found", UI::cDim()); UI::footer("[ENTER] rescan  [`] back"); return; }
    std::vector<std::string> rows;
    for (int i = 0; i < _count; ++i) {
        int n = _order[i];
        char line[64];
        String ssid = WiFi.SSID(n);
        if (ssid.isEmpty()) ssid = "<hidden>";
        snprintf(line, sizeof(line), "%.16s %ddBm c%d %s",
                 ssid.c_str(), WiFi.RSSI(n), WiFi.channel(n), encStr(WiFi.encryptionType(n)));
        rows.push_back(line);
    }
    UI::list(rows, _sel, _top);
    UI::footer("[ENTER] detail  [r] rescan  [`] back");
}

void WifiScan::drawChannels() {
    UI::clear();
    UI::header("WiFi channels", "2.4 GHz");
    auto& d = M5Cardputer.Display;
    int perCh[15] = {0};
    int maxc = 1;
    for (int i = 0; i < _count; ++i) {
        int ch = WiFi.channel(_order[i]);
        if (ch >= 1 && ch <= 14) { perCh[ch]++; if (perCh[ch] > maxc) maxc = perCh[ch]; }
    }
    // 14 columns across the width
    const int x0 = 8, top = 24, bottomY = UI::H - 22, usableH = bottomY - top;
    const int colW = (UI::W - 2 * x0) / 14;
    d.setFont(&fonts::Font0);
    d.setTextDatum(top_center);
    for (int ch = 1; ch <= 14; ++ch) {
        int h = (perCh[ch] * usableH) / maxc;
        int x = x0 + (ch - 1) * colW;
        d.fillRect(x + 1, bottomY - h, colW - 2, h, perCh[ch] ? UI::cAccent() : UI::cLine());
        d.setTextColor(UI::cDim(), UI::cBg());
        d.drawString(String(ch).c_str(), x + colW / 2, bottomY + 2);
        if (perCh[ch]) {
            d.setTextColor(UI::cText(), UI::cBg());
            d.drawString(String(perCh[ch]).c_str(), x + colW / 2, bottomY - h - 9);
        }
    }
    UI::footer("busiest channels have the tallest bars   [`] back");
}

void WifiScan::drawDetail() {
    UI::clear();
    UI::header("AP detail");
    if (_count == 0) { UI::banner("Scan first", UI::cDim()); UI::footer("[`] back"); return; }
    int n = _order[_sel];
    auto& d = M5Cardputer.Display;
    d.setTextDatum(top_left);
    int y = 24;
    String ssid = WiFi.SSID(n); if (ssid.isEmpty()) ssid = "<hidden>";
    d.setFont(&fonts::Font2); d.setTextColor(UI::cAccent(), UI::cBg());
    d.drawString(ssid.c_str(), 8, y); y += 20;
    d.setFont(&fonts::Font0); d.setTextColor(UI::cText(), UI::cBg());
    char l[64];
    snprintf(l, sizeof(l), "BSSID %s", WiFi.BSSIDstr(n).c_str()); d.drawString(l, 8, y); y += 13;
    snprintf(l, sizeof(l), "Channel %d", WiFi.channel(n));        d.drawString(l, 8, y); y += 13;
    snprintf(l, sizeof(l), "Signal  %d dBm", WiFi.RSSI(n));       d.drawString(l, 8, y); y += 13;
    snprintf(l, sizeof(l), "Security %s", encStr(WiFi.encryptionType(n))); d.drawString(l, 8, y); y += 13;
    UI::footer("[;/.] other AP   [`] back");
}

core::Signal WifiScan::onTick(const core::KeyEvent& ev) {
    switch (_view) {
    case View::Menu:
        if (ev.key == Key::Left)  { _menu.move(-1, 0); _menu.draw("WiFi"); }
        if (ev.key == Key::Right) { _menu.move(+1, 0); _menu.draw("WiFi"); }
        if (ev.key == Key::Up)    { _menu.move(0, -1); _menu.draw("WiFi"); }
        if (ev.key == Key::Down)  { _menu.move(0, +1); _menu.draw("WiFi"); }
        if (ev.key == Key::Back) return Signal::Back;
        if (ev.key == Key::Ok) {
            scan();
            if (_menu.selection() == 0) { _view = View::List;     drawList(); }
            else if (_menu.selection() == 1) { _view = View::Channels; drawChannels(); }
            else { _view = View::Detail; drawDetail(); }
        }
        break;

    case View::List:
        if (ev.key == Key::Back) { _view = View::Menu; _menu.draw("WiFi"); break; }
        if (ev.key == Key::Char && (ev.ch == 'r' || ev.ch == 'R')) { scan(); drawList(); }
        if (_count > 0) {
            if (ev.key == Key::Up)   { UI::move(-1, _count, _sel, _top); drawList(); }
            if (ev.key == Key::Down) { UI::move(+1, _count, _sel, _top); drawList(); }
            if (ev.key == Key::Ok)   { _view = View::Detail; drawDetail(); }
        }
        break;

    case View::Channels:
        if (ev.key == Key::Back) { _view = View::Menu; _menu.draw("WiFi"); break; }
        if (ev.key == Key::Ok)   { scan(); drawChannels(); }
        break;

    case View::Detail:
        if (ev.key == Key::Back) { _view = View::List; drawList(); break; }
        if (_count > 0) {
            if (ev.key == Key::Up)   { UI::move(-1, _count, _sel, _top); drawDetail(); }
            if (ev.key == Key::Down) { UI::move(+1, _count, _sel, _top); drawDetail(); }
        }
        break;
    }
    return Signal::Stay;
}

}  // namespace apps
