// =============================================================================
// nfc.h  -  13.56 MHz NFC / RFID tool (ST25R3916 on the Cap)
//
// Icon sub-menu (for YOUR OWN tags/cards):
//   * READ    : detect + identify a PICC, show UID / type / ATQA / SAK
//   * CONTENT : decode and show the NDEF message - text, URLs, and small
//               images (image/png) - not just the UID
//   * DUMP    : full memory dump over USB serial
//   * SAVE    : append the last tag's identity to a CSV on the microSD card
//
// Built on M5Stack's M5Unit-NFC (M5UnitUnified) for the Cap's SPI ST25R3916.
// =============================================================================
#pragma once
#include "../core/app.h"
#include "../core/ui.h"
#include "../core/iconmenu.h"
#include <string>

namespace apps {

class Nfc : public core::App {
public:
    const char* title() const override { return "NFC / RFID"; }
    void   onEnter() override;
    core::Signal onTick(const core::KeyEvent& ev) override;
    void   onExit() override;

private:
    enum class View { Menu, Result };
    View _view = View::Menu;
    core::IconMenu _menu;
    bool _menuBuilt = false;

    bool _ready = false;
    bool ensureReader();
    void drawMenu();
    void readOnce();             // identity only
    void readContent();          // NDEF text / URL / image
    void dumpSerial();
    void saveIdentity();

    std::string _lastUid, _lastType;
    unsigned    _lastAtqa = 0, _lastSak = 0, _lastSize = 0;
    bool        _haveTag = false;
};

}  // namespace apps
