// =============================================================================
// input.h  -  Keyboard abstraction over M5Cardputer's keyboard
//
// Turns the raw key matrix into a small set of logical events the apps use:
// Up/Down/Left/Right/Ok/Back plus the raw typed character. Works on both the
// original Cardputer (matrix) and the Cardputer Adv (TCA8418) because
// M5Cardputer.Keyboard already abstracts the two.
// =============================================================================
#pragma once
#include <M5Cardputer.h>

namespace core {

enum class Key { None, Up, Down, Left, Right, Ok, Back, Char };

struct KeyEvent {
    Key  key = Key::None;
    char ch  = 0;      // valid when key == Key::Char
};

class Input {
public:
    // Poll once per loop. Returns an edge-triggered event (one per physical
    // press); holding a key does not autorepeat, which keeps menus sane.
    static KeyEvent poll() {
        M5Cardputer.update();
        KeyEvent ev;
        if (!M5Cardputer.Keyboard.isChange()) return ev;
        if (!M5Cardputer.Keyboard.isPressed()) return ev;  // only act on key-down

        auto st = M5Cardputer.Keyboard.keysState();

        // M5Cardputer 1.1.1's KeysState exposes enter / del / space + the typed
        // characters (word); it has no up/down/left/right/esc flags. So we map
        // navigation from the character keys:
        //   ; = up   . = down   , = left   / = right   ` = back   (Del also = back)
        if (st.enter) { ev.key = Key::Ok;   return ev; }
        if (st.del)   { ev.key = Key::Back; return ev; }

        for (char c : st.word) {
            switch (c) {
                case ';': ev.key = Key::Up;    return ev;
                case '.': ev.key = Key::Down;  return ev;
                case ',': ev.key = Key::Left;  return ev;
                case '/': ev.key = Key::Right; return ev;
                case '`': ev.key = Key::Back;  return ev;
                default:  ev.key = Key::Char; ev.ch = c; return ev;
            }
        }
        return ev;
    }
};

}  // namespace core
