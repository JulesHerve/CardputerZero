// =============================================================================
// ble.h  -  Bluetooth LE tool with an icon sub-menu
//
//   SCAN      : passive list of nearby BLE advertisers (listen only)
//   KEYBOARD  : the Cardputer becomes a BLE HID keyboard - pair it from your
//               own PC / phone / tablet and whatever you type here is sent to
//               that device over Bluetooth (the wireless sibling of USB macro)
// =============================================================================
#pragma once
#include "../core/app.h"
#include "../core/ui.h"
#include "../core/iconmenu.h"
#include <vector>
#include <string>

namespace apps {

class Ble : public core::App {
public:
    const char* title() const override { return "BLE"; }
    void   onEnter() override;
    core::Signal onTick(const core::KeyEvent& ev) override;
    void   onExit() override;

private:
    enum class View { Menu, Scan, Keyboard };
    View _view = View::Menu;
    core::IconMenu _menu;
    bool _menuBuilt = false;

    // scan
    int _sel = 0, _top = 0;
    std::vector<std::string> _rows;
    void doScan();
    void drawScan();

    // keyboard
    void kbStart();
    void kbStop();
    void kbDraw();
    void kbKey(const core::KeyEvent& ev);

    void ensureBle();            // BLEDevice::init once, with the HID name
    void stopRadio();            // deinit BLE when leaving the whole app
    bool _radioUp   = false;
    bool _hidBuilt  = false;     // HID server/service created once
};

}  // namespace apps
