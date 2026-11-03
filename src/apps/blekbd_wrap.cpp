// =============================================================================
// blekbd_wrap.cpp  -  implementation; includes ONLY BleKeyboard.h (no M5 header)
// so the library's KEY_* constants never meet M5Cardputer's KEY_* macros.
// =============================================================================
#include "blekbd_wrap.h"
#include <BleKeyboard.h>

namespace {
BleKeyboard g_kb("CardputerZero", "CardputerZero", 100);
}

namespace blekbd {

void begin()        { g_kb.begin(); }
void end()          { g_kb.end(); }
bool isConnected()  { return g_kb.isConnected(); }
void writeAscii(uint8_t c) { g_kb.write(c); }

void writeSpecial(Special s) {
    switch (s) {
        case Special::Enter: g_kb.write(KEY_RETURN);      break;
        case Special::Up:    g_kb.write(KEY_UP_ARROW);    break;
        case Special::Down:  g_kb.write(KEY_DOWN_ARROW);  break;
        case Special::Left:  g_kb.write(KEY_LEFT_ARROW);  break;
        case Special::Right: g_kb.write(KEY_RIGHT_ARROW); break;
    }
}

}  // namespace blekbd
