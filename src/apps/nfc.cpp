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

// --- live diagnostics (shown on the DIAG screen; -1 == not attempted yet) ----
int g_diagAddSpi   = -1;
int g_diagBegin    = -1;
int g_diagDetect   = -1;
int g_diagIdentify = -1;

// --- tag emulation (mirrors M5's NFCA/Emulation example) ---
class EmuA : public m5::nfc::EmulationLayerA {
public:
    using m5::nfc::EmulationLayerA::EmulationLayerA;   // inherit ctors
};
EmuA     g_emu{g_unit};
bool     g_haveEmu = false;
Type     g_emuType{};
uint8_t  g_emuUid[10] = {0};
uint8_t  g_emuUidLen = 0;
uint8_t  g_emuMem[180] = {0};
uint16_t g_emuMemLen = 0;        // bytes of real tag memory captured (0 = UID only)

uint8_t bcc8(const uint8_t* p, uint8_t len, uint8_t init = 0) {
    uint8_t v = init;
    for (uint8_t i = 0; i < len; ++i) v ^= p[i];
    return v;
}
void embedUid7(uint8_t mem[9], const uint8_t uid[7]) {
    memcpy(mem, uid, 3);
    mem[3] = bcc8(uid, 3, 0x88);     // CT
    memcpy(mem + 4, uid + 3, 4);
    mem[8] = bcc8(uid + 3, 4);
}

void rememberForEmu(const m5::nfc::a::PICC& picc) {
    g_emuType = picc.type;
    g_emuUidLen = picc.size <= 10 ? picc.size : 10;
    memcpy(g_emuUid, picc.uid, g_emuUidLen);
    g_haveEmu = true;
}
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
        bool wired = m5::unit::wiring::addSPI(g_units, g_unit, 10000000, 1);
        bool begun = wired && g_units.begin();
        Serial.printf("[NFC] addSPI=%d begin=%d\n", (int)wired, (int)begun);
        g_diagAddSpi = (int)wired;
        g_diagBegin  = (int)begun;
        g_begun = begun;
    }
    _ready = g_begun;
    return _ready;
}

void Nfc::onEnter() {
    _view = View::Menu;
    if (!_menuBuilt) {
        _menu.add(core::Icon::Read,    "READ",    "UID, type, ATQA / SAK");
        _menu.add(core::Icon::Dump,    "CONTENT", "Decode NDEF text / URL / image");
        _menu.add(core::Icon::Tx,      "EMULATE", "Present the last-read tag over RF");
        _menu.add(core::Icon::Analyze, "DUMP",    "Memory dump to USB + SD summary");
        _menu.add(core::Icon::Info,    "DIAG",    "On-screen reader status (no serial)");
        _menu.add(core::Icon::Save,    "SAVE",    "Log last tag to SD (CSV)");
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
    bool det = g_nfc.detect(picc);
    bool idn = det && g_nfc.identify(picc);
    g_diagDetect = (int)det; g_diagIdentify = (int)idn;
    Serial.printf("[NFC] detect=%d identify=%d uid=%s type=%s\n",
                  (int)det, (int)idn,
                  idn ? picc.uidAsString().c_str() : "-",
                  idn ? picc.typeAsString().c_str() : "-");
    if (det && idn) {
        M5.Speaker.tone(3000, 20);
        _lastUid  = picc.uidAsString();
        _lastType = picc.typeAsString();
        _lastAtqa = picc.atqa;
        _lastSak  = picc.sak;
        _lastSize = picc.size;
        _haveTag  = true;
        rememberForEmu(picc);
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
    rememberForEmu(picc);

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
    _view = View::Menu;          // return control to the icon menu (fixes freeze)
    drawMenu();
}

void Nfc::emulate() {
    if (!g_haveEmu) { UI::banner("Read a tag first", UI::cWarn()); delay(900); drawMenu(); return; }
    if (!ensureReader()) { UI::banner("Reader init failed", UI::cWarn()); delay(1200); drawMenu(); return; }
    SpiBus::claim(SpiBus::Owner::Nfc);

    // Build the PICC to present and a small backing memory with the UID embedded.
    PICC epicc{};
    epicc.emulate(g_emuType, g_emuUid, g_emuUidLen);
    // If we captured the real tag memory at read time, present that. Otherwise
    // fall back to a UID-only image.
    if (g_emuMemLen == 0) {
        memset(g_emuMem, 0, sizeof(g_emuMem));
        if (g_emuUidLen == 7) embedUid7(g_emuMem, g_emuUid);
        else                  memcpy(g_emuMem, g_emuUid, g_emuUidLen);
    }
    bool ok = g_emu.begin(epicc, g_emuMem, sizeof(g_emuMem));
    UI::clear();
    UI::header("NFC Emulate", _lastType.c_str());
    if (!ok) { UI::banner("Emulation unsupported\nfor this tag", UI::cWarn()); delay(1200); g_emu.end(); drawMenu(); return; }

    auto& d = M5Cardputer.Display;
    UI::banner(("Emulating\n" + _lastUid).c_str(), UI::cAccent());
    UI::footer("present to a reader   [`] stop");

    EmuA::State last = EmuA::State::None;
    uint32_t start = millis();
    while (true) {
        g_units.update();
        g_emu.update();                    // must be pumped continuously
        EmuA::State st = g_emu.state();
        if (st != last) {                  // show live state in the footer
            last = st;
            const char* n = "idle";
            switch (st) {
                case EmuA::State::Active: n = "ACTIVE (being read)"; break;
                case EmuA::State::Ready:  n = "ready"; break;
                case EmuA::State::Halt:   n = "halt"; break;
                case EmuA::State::Idle:   n = "waiting for reader"; break;
                default:                  n = "waiting for reader"; break;
            }
            d.fillRect(0, UI::H - 14, UI::W, 14, UI::cBar());
            d.setFont(&fonts::Font0); d.setTextDatum(middle_left);
            d.setTextColor(UI::cDim(), UI::cBar());
            d.drawString(n, 8, UI::H - 7);
        }
        auto ev = Input::poll();
        if (ev.key == Key::Back) break;
        if (millis() - start > 120000) break;   // safety timeout
        delay(2);
    }
    g_emu.end();
    g_nfc.deactivate();
    drawMenu();
}

void Nfc::dumpSerial() {
    if (!ensureReader()) { UI::banner("Reader init failed", UI::cWarn()); delay(1200); drawMenu(); return; }
    SpiBus::claim(SpiBus::Owner::Nfc);
    g_units.update();
    UI::banner("Dumping to USB serial...", UI::cAccent());

    PICC picc{};
    g_diagDetect = (int)g_nfc.detect(picc);
    if (g_diagDetect && g_nfc.identify(picc) && g_nfc.reactivate(picc)) {
        g_diagIdentify = 1;
        Serial.printf("\n==== NFC dump %s %s ====\n",
                      picc.uidAsString().c_str(), picc.typeAsString().c_str());
        g_nfc.dump(m5::nfc::a::mifare::classic::DEFAULT_KEY);
        g_nfc.deactivate();
        // Many users can't open a USB serial monitor, so also drop a short
        // summary onto the SD card they CAN read on a computer.
        bool saved = false;
        if (Storage::isReady()) {
            char meta[96];
            snprintf(meta, sizeof(meta), "%s,%s,ATQA=%04X,SAK=%02X,%u-byte\n",
                     picc.typeAsString().c_str(), picc.uidAsString().c_str(),
                     picc.atqa, picc.sak, picc.size);
            saved = Storage::append(std::string(cfg::DIR_NFC) + "/dumps.csv", meta);
        }
        UI::banner(saved ? "Dump on USB serial\nsummary -> /czero/nfc/dumps.csv"
                         : "Dump complete\n(see USB serial)", UI::cAccent());
    } else {
        g_diagIdentify = 0;
        UI::banner("No tag / identify failed", UI::cWarn());
    }
    delay(1400);
    drawMenu();
}

void Nfc::drawDiag() {
    ensureReader();                    // attempt init so the status is live
    auto& d = M5Cardputer.Display;
    UI::clear();
    UI::header("NFC diagnostics", _ready ? "reader OK" : "reader FAIL");
    d.setFont(&fonts::Font0);
    d.setTextDatum(top_left);
    int y = 22;
    auto yn = [](int v) -> const char* { return v < 0 ? "-" : (v ? "yes" : "NO"); };
    char l[64];

    d.setTextColor(_ready ? UI::cText() : UI::cWarn(), UI::cBg());
    snprintf(l, sizeof(l), "init  addSPI:%s  begin:%s", yn(g_diagAddSpi), yn(g_diagBegin));
    d.drawString(l, 6, y); y += 13;

    d.setTextColor(UI::cDim(), UI::cBg());
    snprintf(l, sizeof(l), "SPI CS=G%d  IRQ=G%d  shared bus", cfg::PIN_NFC_CS, cfg::PIN_NFC_IRQ);
    d.drawString(l, 6, y); y += 13;
    snprintf(l, sizeof(l), "SD card: %s", Storage::isReady() ? "ready" : "none");
    d.drawString(l, 6, y); y += 13;

    d.setTextColor(UI::cText(), UI::cBg());
    snprintf(l, sizeof(l), "last read  detect:%s  id:%s", yn(g_diagDetect), yn(g_diagIdentify));
    d.drawString(l, 6, y); y += 13;

    if (_haveTag) {
        d.setTextColor(UI::cAccent(), UI::cBg());
        d.drawString(("UID " + _lastUid).c_str(), 6, y); y += 13;
        d.setTextColor(UI::cDim(), UI::cBg());
        d.drawString(_lastType.c_str(), 6, y);
    } else {
        d.setTextColor(UI::cDim(), UI::cBg());
        d.drawString("no tag read yet - run READ first", 6, y);
    }
    UI::footer("[r] re-test reader   [`] back");
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
            case 2: emulate(); break;
            case 3: dumpSerial(); break;
            case 4: _view = View::Diag; drawDiag(); break;
            case 5: saveIdentity(); break;
            }
        }
        break;
    case View::Result:
        if (ev.key == Key::Char && (ev.ch == 's' || ev.ch == 'S')) saveIdentity();
        if (ev.key == Key::Back || ev.key == Key::Ok) { _view = View::Menu; drawMenu(); }
        break;
    case View::Diag:
        if (ev.key == Key::Char && (ev.ch == 'r' || ev.ch == 'R')) {
            ensureReader();        // retry init if it hasn't come up yet
            drawDiag();
        }
        if (ev.key == Key::Back || ev.key == Key::Ok) { _view = View::Menu; drawMenu(); }
        break;
    }
    return Signal::Stay;
}

}  // namespace apps
