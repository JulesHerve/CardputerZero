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
    char ch  = 0;      // valid when key == Key::Char; also '\b' on a Back from Del
};

// Both the backtick (`, used as esc/back) and Del produce Key::Back so that menus
// keep going "back" on either key as they always have. The keyboard screens need
// to tell them apart (esc exits, Del sends a backspace to the host), so a Back
// that came from Del carries ch == '\b'. Use this to detect it.
inline bool isBackspace(const KeyEvent& ev) { return ev.key == Key::Back && ev.ch == '\b'; }
inline bool isEscape(const KeyEvent& ev)    { return ev.key == Key::Back && ev.ch != '\b'; }

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
        // Del acts as "back" in menus (ch marks it so keyboard screens can instead
        // forward a real backspace to the connected host).
        if (st.del)   { ev.key = Key::Back; ev.ch = '\b'; return ev; }

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
