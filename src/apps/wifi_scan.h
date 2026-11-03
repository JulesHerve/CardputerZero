// =============================================================================
// wifi_scan.h  -  Wi-Fi tool (passive, read-only) with an icon sub-menu
//
//   SCAN      : access points in range, sorted strongest-first
//   CHANNELS  : bar graph of how many APs sit on each 2.4 GHz channel (1-14)
//   DETAIL    : full info for one AP (SSID / BSSID / channel / RSSI / security)
//
// Everything here only *listens*: it never connects, transmits, or interferes.
// =============================================================================
#pragma once
#include "../core/app.h"
#include "../core/ui.h"
#include "../core/iconmenu.h"

namespace apps {

class WifiScan : public core::App {
public:
    const char* title() const override { return "WiFi"; }
    void   onEnter() override;
    core::Signal onTick(const core::KeyEvent& ev) override;
    void   onExit() override;

private:
    enum class View { Menu, List, Channels, Detail };
    View _view = View::Menu;
    core::IconMenu _menu;
    bool _menuBuilt = false;

    int  _count = 0;          // APs found
    int  _sel = 0, _top = 0;  // list cursor
    int  _order[64];          // indices sorted by RSSI (strongest first)

    void scan();              // (re)scan and sort
    void drawList();
    void drawChannels();
    void drawDetail();
};

}  // namespace apps
