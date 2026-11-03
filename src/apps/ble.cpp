// =============================================================================
// ble.cpp  -  implementation of the BLE tool (scan + HID keyboard)
// =============================================================================
#include "ble.h"
#include "../core/input.h"

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "blekbd_wrap.h"     // BleKeyboard behind a wrapper (avoids KEY_* macro clash)

using namespace core;

namespace {
bool g_bleUp = false;     // a BLE role currently holds the controller
bool g_kbUp  = false;     // the keyboard role specifically is active
}  // namespace

namespace apps {

void Ble::onEnter() {
    _view = View::Menu;
    if (!_menuBuilt) {
        _menu.add(core::Icon::Scan,     "SCAN",     "List nearby BLE devices");
        _menu.add(core::Icon::Keyboard, "KEYBOARD", "Be a BLE keyboard for your device");
        _menuBuilt = true;
    }
    _menu.draw("BLE");
}

void Ble::onExit() { stopAllBle(); }

void Ble::stopAllBle() {
    if (g_kbUp) { blekbd::end(); g_kbUp = false; }
    if (g_bleUp) {
        // deinit(false): keep the controller memory so a later init() works.
        BLEDevice::deinit(false);
        g_bleUp = false;
    }
    delay(40);
}

// ---------------- SCAN ----------------
void Ble::doScan() {
    stopAllBle();                       // ensure the keyboard role is gone
    UI::clear(); UI::header("BLE scan"); UI::banner("Scanning 5s...", UI::cAccent());
    BLEDevice::init("");
    g_bleUp = true;

    BLEScan* s = BLEDevice::getScan();
    s->setActiveScan(true);
    s->setInterval(100);
    s->setWindow(99);
    BLEScanResults results = s->start(5, false);   // 2.0.x returns by value

    _rows.clear();
    int n = results.getCount();
    for (int i = 0; i < n; ++i) {
        BLEAdvertisedDevice dev = results.getDevice(i);
        char line[80];
        std::string name = dev.haveName() ? dev.getName() : std::string("<no name>");
        snprintf(line, sizeof(line), "%.16s %ddBm %.17s",
                 name.c_str(), dev.getRSSI(), dev.getAddress().toString().c_str());
        _rows.push_back(line);
    }
    s->clearResults();
    _sel = _top = 0;
    drawScan();
}

void Ble::drawScan() {
    UI::clear();
    char hdr[20]; snprintf(hdr, sizeof(hdr), "%d dev", (int)_rows.size());
    UI::header("BLE scan", hdr);
    if (_rows.empty()) UI::banner("No devices found", UI::cDim());
    else UI::list(_rows, _sel, _top);
    UI::footer("[ENTER] rescan   [`] back");
}

// ---------------- KEYBOARD ----------------
void Ble::kbStart() {
    stopAllBle();                       // free the controller before claiming it
    blekbd::begin();                    // the library inits BLE + HID + advertising
    g_bleUp = true; g_kbUp = true;
    kbDraw(false);
}

void Ble::kbDraw(bool linked) {
    UI::clear();
    UI::header("BLE keyboard", linked ? "linked" : "pairing");
    if (linked) UI::banner("Connected.\nType - keys go to your device.", UI::cAccent());
    else        UI::banner("Pair \"CardputerZero\"\nfrom your device's BT menu", UI::cDim());
    UI::footer("[`] back   type to send");
}

void Ble::kbKey(const core::KeyEvent& ev) {
    if (!g_kbUp || !blekbd::isConnected()) return;
    switch (ev.key) {
        case Key::Ok:    blekbd::writeSpecial(blekbd::Special::Enter); break;
        case Key::Up:    blekbd::writeSpecial(blekbd::Special::Up);    break;
        case Key::Down:  blekbd::writeSpecial(blekbd::Special::Down);  break;
        case Key::Left:  blekbd::writeSpecial(blekbd::Special::Left);  break;
        case Key::Right: blekbd::writeSpecial(blekbd::Special::Right); break;
        case Key::Char:  blekbd::writeAscii((uint8_t)ev.ch);           break;
        default: break;
    }
}

// ---------------- dispatch ----------------
core::Signal Ble::onTick(const core::KeyEvent& ev) {
    switch (_view) {
    case View::Menu:
        if (ev.key == Key::Left)  { _menu.move(-1, 0); _menu.draw("BLE"); }
        if (ev.key == Key::Right) { _menu.move(+1, 0); _menu.draw("BLE"); }
        if (ev.key == Key::Up)    { _menu.move(0, -1); _menu.draw("BLE"); }
        if (ev.key == Key::Down)  { _menu.move(0, +1); _menu.draw("BLE"); }
        if (ev.key == Key::Back) return Signal::Back;
        if (ev.key == Key::Ok) {
            if (_menu.selection() == 0) { _view = View::Scan; doScan(); }
            else                        { _view = View::Keyboard; kbStart(); }
        }
        break;

    case View::Scan:
        if (ev.key == Key::Back) { stopAllBle(); _view = View::Menu; _menu.draw("BLE"); break; }
        if (ev.key == Key::Ok)   doScan();
        if (!_rows.empty()) {
            if (ev.key == Key::Up)   { UI::move(-1, _rows.size(), _sel, _top); drawScan(); }
            if (ev.key == Key::Down) { UI::move(+1, _rows.size(), _sel, _top); drawScan(); }
        }
        break;

    case View::Keyboard: {
        if (ev.key == Key::Back) { stopAllBle(); _view = View::Menu; _menu.draw("BLE"); break; }
        static bool lastLinked = false;
        bool linked = g_kbUp && blekbd::isConnected();
        if (ev.key != Key::None) kbKey(ev);     // send the keystroke
        if (linked != lastLinked) { lastLinked = linked; kbDraw(linked); }
        break;
    }
    }
    return Signal::Stay;
}

}  // namespace apps
