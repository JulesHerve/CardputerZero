// =============================================================================
// wifi_scan.h  -  Wi-Fi tool with an icon sub-menu
//
//   SCAN      : access points in range, sorted strongest-first   (read-only)
//   CHANNELS  : bar graph of AP count per 2.4 GHz channel (1-14)  (read-only)
//   DETAIL    : full info for one AP                              (read-only)
//   PORTAL    : run YOUR OWN SoftAP + a guest sign-in page that saves visitors'
//               names to /czero/wifi/guests.csv                   (you operate it)
//   PUN AP    : broadcast ONE SoftAP named from a list of Wi-Fi puns
//
// SCAN/CHANNELS/DETAIL only listen. PORTAL and PUN AP stand up the Cardputer's
// own access point (SoftAP) - they advertise a network you name; they do not
// touch, spoof, or interfere with anyone else's network.
// =============================================================================
#pragma once
#include "../core/app.h"
#include "../core/ui.h"
#include "../core/iconmenu.h"
#include <string>

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

    std::string _ssid = "CardputerZero-Guest";   // last SoftAP name used

    void scan();              // (re)scan and sort
    void drawList();
    void drawChannels();
    void drawDetail();

    // SoftAP features (each runs its own blocking loop, then returns to Menu).
    bool editText(const char* title, std::string& io, size_t maxLen);
    void runPortal();         // guest sign-in captive portal on your own AP
    void runPunAp();          // broadcast one pun-named AP
};

}  // namespace apps
