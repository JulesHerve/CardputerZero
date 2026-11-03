// =============================================================================
// nfc.cpp  -  implementation of the NFC / RFID tool
// =============================================================================
#include "nfc.h"
#include "../core/spi_bus.h"
#include "../core/storage.h"
#include "../config.h"

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedNFC.h>
#include <M5Utility.h>
#include <wiring/m5_unit_unified_wiring.hpp>
#include <vector>
#include <cstring>

using namespace core;
using namespace m5::nfc;
using namespace m5::nfc::a;
using namespace m5::nfc::ndef;

namespace {
m5::unit::UnitUnified   g_units;
m5::unit::CapCC1101NFC  g_unit{};
m5::nfc::NFCLayerA      g_nfc{g_unit};
bool                    g_begun = false;
}  // namespace

namespace apps {

bool Nfc::ensureReader() {
    if (_ready) return true;
    pinMode(cfg::PIN_CAP_POWER_EN, OUTPUT);
    digitalWrite(cfg::PIN_CAP_POWER_EN, HIGH);
    delay(20);
    SpiBus::claim(SpiBus::Owner::Nfc);
    if (!g_begun) {
        // SPI mode 1 @ 10 MHz, exactly as M5Stack's CapCC1101 examples use.
        bool ok = m5::unit::wiring::addSPI(g_units, g_unit, 10000000, 1) && g_units.begin();
        g_begun = ok;
    }
    _ready = g_begun;
    return _ready;
}

void Nfc::onEnter() {
    _view = View::Menu;
    if (!_menuBuilt) {
        _menu.add(core::Icon::Read, "READ",    "UID, type, ATQA / SAK");
        _menu.add(core::Icon::Dump, "CONTENT", "Decode NDEF text / URL / image");
        _menu.add(core::Icon::Info, "SERIAL",  "Full memory dump over USB");
        _menu.add(core::Icon::Save, "SAVE",    "Log last tag to SD (CSV)");
        _menuBuilt = true;
    }
    drawMenu();
}

void Nfc::onExit() { SpiBus::parkAll(); }

void Nfc::drawMenu() {
    _menu.draw("NFC / RFID", Storage::isReady() ? "SD ok" : "no SD");
}

void Nfc::readOnce() {
    if (!ensureReader()) { UI::banner("Reader init failed", UI::cWarn()); delay(1200); drawMenu(); return; }
    SpiBus::claim(SpiBus::Owner::Nfc);
    UI::banner("Hold a tag to the\nback of the Cap", UI::cAccent());
    g_units.update();

    PICC picc{};
    if (g_nfc.detect(picc) && g_nfc.identify(picc)) {
        M5.Speaker.tone(3000, 20);
        _lastUid  = picc.uidAsString();
        _lastType = picc.typeAsString();
        _lastAtqa = picc.atqa;
        _lastSak  = picc.sak;
        _lastSize = picc.size;
        _haveTag  = true;
        g_nfc.deactivate();

        auto& d = M5Cardputer.Display;
        UI::clear();
        UI::header("Tag identity");
        d.setFont(&fonts::Font2);
        d.setTextDatum(top_left);
        int y = 24;
        d.setTextColor(UI::cAccent(), UI::cBg()); d.drawString(_lastType.c_str(), 8, y); y += 20;
        d.setTextColor(UI::cText(), UI::cBg());
        d.drawString(("UID " + _lastUid).c_str(), 8, y); y += 18;
        char l[48]; snprintf(l, sizeof(l), "ATQA %04X  SAK %02X  %u-byte", _lastAtqa, _lastSak, _lastSize);
        d.setFont(&fonts::Font0); d.setTextColor(UI::cDim(), UI::cBg());
        d.drawString(l, 8, y);
        UI::footer("[s] save   [`] back");
        _view = View::Result;
    } else {
        UI::banner("No tag found", UI::cWarn());
        delay(800);
        drawMenu();
    }
}

void Nfc::readContent() {
    if (!ensureReader()) { UI::banner("Reader init failed", UI::cWarn()); delay(1200); drawMenu(); return; }
    SpiBus::claim(SpiBus::Owner::Nfc);
    UI::banner("Reading content...", UI::cAccent());
    g_units.update();

    auto& d = M5Cardputer.Display;
    PICC picc{};
    if (!(g_nfc.detect(picc) && g_nfc.identify(picc) && g_nfc.reactivate(picc))) {
        UI::banner("No tag found", UI::cWarn()); delay(800); drawMenu(); return;
    }
    M5.Speaker.tone(3000, 20);
    _lastUid = picc.uidAsString(); _lastType = picc.typeAsString();
    _lastAtqa = picc.atqa; _lastSak = picc.sak; _lastSize = picc.size; _haveTag = true;

    if (!picc.supportsNDEF()) {
        g_nfc.deactivate();
        UI::clear(); UI::header("NFC Content");
        UI::banner("Tag has no NDEF\n(UID only)", UI::cDim());
        UI::footer("[`] back"); _view = View::Result; return;
    }

    bool valid = false;
    if (!g_nfc.ndefIsValidFormat(valid) || !valid) {
        g_nfc.deactivate();
        UI::clear(); UI::header("NFC Content");
        UI::banner("Not NDEF formatted", UI::cWarn());
        UI::footer("[`] back"); _view = View::Result; return;
    }

    TLV msg{};
    bool readOk = g_nfc.ndefRead(msg);
    g_nfc.deactivate();

    UI::clear();
    UI::header("NFC Content");
    if (!readOk || !msg.isMessageTLV()) {
        UI::banner("Empty / unreadable NDEF", UI::cDim());
        UI::footer("[`] back"); _view = View::Result; return;
    }

    d.setTextDatum(top_left);
    int y = 22;
    bool drewImage = false;
    for (auto&& r : msg.records()) {
        if (y > UI::H - 26) break;                       // ran out of screen
        const char* t = r.type();
        if (r.tnf() == TNF::MIMEMedia && t && strcmp(t, "image/png") == 0) {
            // Render a small embedded PNG image from the tag.
            d.drawPng(r.payload(), r.payloadSize(), 4, y);
            drewImage = true;
            y += 70;
            continue;
        }
        std::string body = r.payloadAsString();          // text -> text, URI -> full URL
        const char* kind = "DATA";
        if (r.tnf() == TNF::Wellknown && t) {
            if (!strcmp(t, "T")) kind = "TEXT";
            else if (!strcmp(t, "U")) kind = "LINK";
        } else if (t && *t) {
            kind = t;                                    // MIME type, e.g. text/plain
        }
        d.setFont(&fonts::Font0);
        d.setTextColor(UI::cAccent(), UI::cBg());
        d.drawString(kind, 6, y); y += 11;
        d.setTextColor(UI::cText(), UI::cBg());
        // wrap the body across lines (~38 chars per line in Font0)
        const size_t WRAP = 38;
        if (body.empty()) body = "(" + std::to_string(r.payloadSize()) + " bytes)";
        for (size_t i = 0; i < body.size() && y <= UI::H - 24; i += WRAP) {
            d.drawString(body.substr(i, WRAP).c_str(), 6, y);
            y += 11;
        }
        y += 4;
    }
    if (drewImage) { /* image already shown above the text */ }
    UI::footer("[`] back");
    _view = View::Result;
}

void Nfc::saveIdentity() {
    if (!_haveTag) { UI::banner("Read a tag first", UI::cWarn()); delay(700); drawMenu(); return; }
    if (!Storage::isReady()) { UI::banner("No SD card", UI::cWarn()); delay(700); drawMenu(); return; }
    std::string line = _lastType + "," + _lastUid + ",";
    char meta[32]; snprintf(meta, sizeof(meta), "ATQA=%04X,SAK=%02X\n", _lastAtqa, _lastSak);
    line += meta;
    bool ok = Storage::append(std::string(cfg::DIR_NFC) + "/tags.csv", line);
    UI::banner(ok ? "Saved to /czero/nfc/tags.csv" : "Save failed",
               ok ? UI::cAccent() : UI::cWarn());
    delay(900);
    drawMenu();
}

void Nfc::dumpSerial() {
    if (!ensureReader()) { UI::banner("Reader init failed", UI::cWarn()); delay(1200); drawMenu(); return; }
    SpiBus::claim(SpiBus::Owner::Nfc);
    g_units.update();
    UI::banner("Dumping to USB serial...", UI::cAccent());

    PICC picc{};
    if (g_nfc.detect(picc) && g_nfc.identify(picc) && g_nfc.reactivate(picc)) {
        Serial.printf("\n==== NFC dump %s %s ====\n",
                      picc.uidAsString().c_str(), picc.typeAsString().c_str());
        g_nfc.dump(m5::nfc::a::mifare::classic::DEFAULT_KEY);
        g_nfc.deactivate();
        UI::banner("Dump complete\n(see USB serial)", UI::cAccent());
    } else {
        UI::banner("No tag / identify failed", UI::cWarn());
    }
    delay(1200);
    drawMenu();
}

core::Signal Nfc::onTick(const core::KeyEvent& ev) {
    switch (_view) {
    case View::Menu:
        if (ev.key == Key::Left)  { _menu.move(-1, 0); drawMenu(); }
        if (ev.key == Key::Right) { _menu.move(+1, 0); drawMenu(); }
        if (ev.key == Key::Up)    { _menu.move(0, -1); drawMenu(); }
        if (ev.key == Key::Down)  { _menu.move(0, +1); drawMenu(); }
        if (ev.key == Key::Back) return Signal::Back;
        if (ev.key == Key::Ok) {
            switch (_menu.selection()) {
            case 0: readOnce(); break;
            case 1: readContent(); break;
            case 2: dumpSerial(); break;
            case 3: saveIdentity(); break;
            }
        }
        break;
    case View::Result:
        if (ev.key == Key::Char && (ev.ch == 's' || ev.ch == 'S')) saveIdentity();
        if (ev.key == Key::Back || ev.key == Key::Ok) { _view = View::Menu; drawMenu(); }
        break;
    }
    return Signal::Stay;
}

}  // namespace apps
