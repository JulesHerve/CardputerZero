// =============================================================================
// blekbd_wrap.h  -  thin wrapper around the ESP32-BLE-Keyboard library
//
// BleKeyboard.h declares `const uint8_t KEY_LEFT_CTRL = 0x80;` (and friends),
// which collide with M5Cardputer's `#define KEY_LEFT_CTRL 0x80` macros. This
// wrapper keeps BleKeyboard.h in its own translation unit that never includes
// any M5 header, exposing only plain functions to the rest of the firmware.
// =============================================================================
#pragma once
#include <cstdint>

namespace blekbd {
enum class Special { Enter, Up, Down, Left, Right };

void begin();                 // init BLE + HID + start advertising
void end();                   // stop (BLE teardown is done by the caller)
bool isConnected();
void writeAscii(uint8_t c);   // a printable ASCII char
void writeSpecial(Special s); // Enter / arrow keys
}  // namespace blekbd
