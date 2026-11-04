// =============================================================================
// main.cpp  -  CardputerZero entry point and home screen
//
// A Flipper-Zero-style multi-tool for the M5Stack Cardputer Adv + Cap CC1101 &
// NFC. Boots into a "Nothing OS"-styled home screen: an iOS-like grid of
// rounded app icons, each with a label, over a pure-black canvas with a single
// red accent. Selecting an icon hands the whole screen to one App until it
// returns Signal::Back.
//
// Hardware + libraries are all M5Stack-official (see docs/ and platformio.ini).
// Everything here is for working with devices, remotes, tags and computers you
// own or are authorised to test. See docs/SAFETY_AND_LEGAL.md.
// =============================================================================
#include <M5Cardputer.h>
#include <M5Unified.h>

#include "core/ui.h"
#include "core/theme.h"
#include "core/input.h"
#include "core/app.h"
#include "core/spi_bus.h"
#include "core/storage.h"
#include "core/iconmenu.h"

#include "apps/subghz.h"
#include "apps/nfc.h"
#include "apps/ir.h"
#include "apps/wifi_scan.h"
#include "apps/ble.h"
#include "apps/badusb.h"

using namespace core;

namespace {

// ---- Tool roster -----------------------------------------------------------
apps::SubGhz   appSubGhz;
apps::Nfc      appNfc;
apps::Ir       appIr;
apps::WifiScan appWifi;
apps::Ble      appBle;
apps::BadUsb   appBadUsb;

App* const APPS[] = { &appSubGhz, &appNfc, &appIr, &appWifi, &appBle, &appBadUsb };
constexpr int N_APPS = sizeof(APPS) / sizeof(APPS[0]);

IconMenu g_home;
App*     g_active = nullptr;     // nullptr => on the home screen

const char* batteryHint() {
    static char buf[12];
    int pct = M5.Power.getBatteryLevel();
    snprintf(buf, sizeof(buf), "%d%%", pct);
    return buf;
}

// --- theme persistence (/czero/theme.txt holds a single digit 0/1/2) ---------
constexpr const char* THEME_FILE = "/czero/theme.txt";

void loadTheme() {
    std::string s;
    if (Storage::isReady() && Storage::read(THEME_FILE, s) && !s.empty()) {
        int v = s[0] - '0';
        if (v >= 0 && v < THEME_COUNT) setTheme((ThemeId)v);
    }
}

void saveTheme() {
    if (!Storage::isReady()) return;
    char v[2] = { (char)('0' + (int)currentTheme()), 0 };
    Storage::write(THEME_FILE, v);
}

void drawHome() { g_home.draw("CardputerZero", batteryHint()); }

void enterApp(int idx) {
    g_active = APPS[idx];
    g_active->onEnter();
}

void leaveApp() {
    if (g_active) g_active->onExit();
    g_active = nullptr;
    SpiBus::parkAll();
    drawHome();
}

}  // namespace

void setup() {
    auto m5cfg = M5.config();
    M5Cardputer.begin(m5cfg, /*enableKeyboard=*/true);
    M5Cardputer.Display.setRotation(1);           // landscape 240x135
    M5Cardputer.Display.setBrightness(180);

    Serial.begin(115200);

    SpiBus::begin();
    Storage::begin();
    loadTheme();                 // apply the saved skin (default = Nothing)

    g_home.add(Icon::SubGhz, "SUB-GHZ", "Capture & replay 315-915 MHz");
    g_home.add(Icon::Nfc,    "NFC",     "Read tags & NDEF content");
    g_home.add(Icon::Ir,     "IR",      "Learn & replay IR remotes");
    g_home.add(Icon::Wifi,   "WI-FI",   "Scan, guest portal, pun AP");
    g_home.add(Icon::Ble,    "BLE",     "Scan or be a BLE keyboard");
    g_home.add(Icon::Usb,    "USB",     "Bad-USB scripts or live keyboard");

    // Splash.
    UI::clear();
    UI::header("CardputerZero", themeName(currentTheme()));
    UI::banner("multi-tool", UI::cAccent());
    UI::footer(Storage::isReady() ? "[t] theme   microSD detected"
                                  : "[t] theme   no microSD (screen-only)");
    delay(900);

    drawHome();
}

void loop() {
    KeyEvent ev = Input::poll();

    if (g_active == nullptr) {
        switch (ev.key) {
            case Key::Left:  g_home.move(-1, 0); drawHome(); break;
            case Key::Right: g_home.move(+1, 0); drawHome(); break;
            case Key::Up:    g_home.move(0, -1); drawHome(); break;
            case Key::Down:  g_home.move(0, +1); drawHome(); break;
            case Key::Ok:    enterApp(g_home.selection()); break;
            case Key::Char:
                // 't' cycles the UI skin (Nothing -> Dell -> Nintendo) and saves it.
                if (ev.ch == 't' || ev.ch == 'T') {
                    ThemeId t = nextTheme();
                    saveTheme();
                    UI::clear();
                    UI::header("Theme");
                    UI::banner(themeName(t), UI::cAccent());
                    UI::footer("[t] next theme   [`] back");
                    delay(650);
                    drawHome();
                }
                break;
            default: break;
        }
    } else {
        if (g_active->onTick(ev) == Signal::Back) leaveApp();
    }
    delay(2);
}
