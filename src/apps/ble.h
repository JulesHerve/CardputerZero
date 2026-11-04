// =============================================================================
// ble.h  -  Bluetooth LE tool with an icon sub-menu
//
//   SCAN      : passive list of nearby BLE advertisers (listen only)
//   KEYBOARD  : the Cardputer becomes a BLE HID keyboard (via the proven
//               ESP32-BLE-Keyboard library) - pair it from your own device and
//               type to it wirelessly.
//
// Scan and keyboard are run one-at-a-time with a full BLE teardown between
// them, so the two Bluetooth roles never fight over the controller.
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
    void kbDraw(bool linked, bool ready);
    void kbKey(const core::KeyEvent& ev);

    void stopAllBle();           // tear down whatever BLE role was active
};

}  // namespace apps
