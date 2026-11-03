// =============================================================================
// ble.cpp  -  implementation of the BLE tool (scan + HID keyboard)
// =============================================================================
#include "ble.h"
#include "../core/input.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <BLEUtils.h>
#include <HIDTypes.h>
#define US_KEYBOARD 1          // use the US ASCII->HID keymap table
#include <HIDKeyboardTypes.h>

using namespace core;

namespace {

// ---- BLE HID keyboard state ----
BLEServer*         g_server = nullptr;
BLEHIDDevice*      g_hid    = nullptr;
BLECharacteristic* g_input  = nullptr;
volatile bool      g_connected = false;

class SrvCb : public BLEServerCallbacks {
    void onConnect(BLEServer*) override { g_connected = true; }
    void onDisconnect(BLEServer* s) override {
        g_connected = false;
        s->getAdvertising()->start();   // allow re-pairing
    }
};
SrvCb g_srvCb;

// Standard boot-keyboard HID report map, report ID 1, 8-byte input reports
// (byte0 = modifier bitmap, byte1 = reserved, bytes2..7 = up to six key usages).
const uint8_t REPORT_MAP[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
    0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00,
    0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x08, 0x81, 0x03,
    0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65,
    0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00,
    0xC0
};

void sendKey(uint8_t modifier, uint8_t usage) {
    if (!g_input || !g_connected) return;
    uint8_t rep[8] = { modifier, 0, usage, 0, 0, 0, 0, 0 };
    g_input->setValue(rep, 8); g_input->notify();
    delay(8);
    uint8_t rel[8] = { 0 };
    g_input->setValue(rel, 8); g_input->notify();
    delay(8);
}

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

void Ble::onExit() { stopRadio(); }

void Ble::stopRadio() {
    if (_radioUp) {
        BLEDevice::deinit(true);
        _radioUp = false;
        g_server = nullptr; g_hid = nullptr; g_input = nullptr; g_connected = false;
    }
}

// ---------------- SCAN ----------------
void Ble::doScan() {
    UI::clear(); UI::header("BLE scan"); UI::banner("Scanning 5s...", UI::cAccent());
    if (!_radioUp) { BLEDevice::init(""); _radioUp = true; }

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
    if (!_radioUp) { BLEDevice::init("CardputerZero"); _radioUp = true; }
    g_connected = false;
    g_server = BLEDevice::createServer();
    g_server->setCallbacks(&g_srvCb);

    g_hid = new BLEHIDDevice(g_server);
    g_input = g_hid->inputReport(1);
    g_hid->manufacturer(std::string("CardputerZero"));
    g_hid->pnp(0x02, 0xE502, 0xA111, 0x0210);
    g_hid->hidInfo(0x00, 0x01);

    BLESecurity* sec = new BLESecurity();
    sec->setAuthenticationMode(ESP_LE_AUTH_BOND);

    g_hid->reportMap((uint8_t*)REPORT_MAP, sizeof(REPORT_MAP));
    g_hid->startServices();
    g_hid->setBatteryLevel(100);

    BLEAdvertising* adv = g_server->getAdvertising();
    adv->setAppearance(HID_KEYBOARD);
    adv->addServiceUUID(g_hid->hidService()->getUUID());
    adv->setScanResponse(true);
    adv->start();

    kbDraw();
}

void Ble::kbStop() { stopRadio(); }

void Ble::kbDraw() {
    UI::clear();
    UI::header("BLE keyboard", g_connected ? "linked" : "pairing");
    if (g_connected) {
        UI::banner("Connected.\nType - keys go to your device.", UI::cAccent());
    } else {
        UI::banner("Pair \"CardputerZero\"\nfrom your device's BT menu", UI::cDim());
    }
    UI::footer("[`] back   type to send");
}

void Ble::kbKey(const core::KeyEvent& ev) {
    if (!g_connected) return;
    switch (ev.key) {
        case Key::Ok:    sendKey(0, 0x28); return;     // Enter
        case Key::Up:    sendKey(0, 0x52); return;
        case Key::Down:  sendKey(0, 0x51); return;
        case Key::Left:  sendKey(0, 0x50); return;
        case Key::Right: sendKey(0, 0x4F); return;
        case Key::Char: {
            uint8_t ch = (uint8_t)ev.ch;
            if (ch < KEYMAP_SIZE) {
                KEYMAP km = keymap[ch];                // ASCII -> {usage, modifier}
                if (km.usage) sendKey(km.modifier, km.usage);
            }
            return;
        }
        default: return;
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
        if (ev.key == Key::Back) { stopRadio(); _view = View::Menu; _menu.draw("BLE"); break; }
        if (ev.key == Key::Ok)   doScan();
        if (!_rows.empty()) {
            if (ev.key == Key::Up)   { UI::move(-1, _rows.size(), _sel, _top); drawScan(); }
            if (ev.key == Key::Down) { UI::move(+1, _rows.size(), _sel, _top); drawScan(); }
        }
        break;

    case View::Keyboard:
        if (ev.key == Key::Back) { kbStop(); _view = View::Menu; _menu.draw("BLE"); break; }
        // redraw link state on any key, then send it
        kbKey(ev);
        kbDraw();
        break;
    }
    return Signal::Stay;
}

}  // namespace apps
