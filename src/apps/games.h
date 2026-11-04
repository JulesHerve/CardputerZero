// =============================================================================
// games.h  -  A little arcade: Snake, Tetris, Flappy, and a Doom-style raycaster
//
// Each game owns the screen while it runs (its own blocking loop) and returns to
// the Games icon menu on ESC. Rendering uses an off-screen M5Canvas sprite for
// flicker-free frames; input reads the key matrix directly each frame so held
// keys repeat (the normal edge-triggered Input layer does not autorepeat).
//
// Controls (all games): ; up  . down  , left  / right   enter = action
//                        `  = quit to the games menu
// =============================================================================
#pragma once
#include "../core/app.h"
#include "../core/ui.h"
#include "../core/iconmenu.h"

namespace apps {

class Games : public core::App {
public:
    const char* title() const override { return "Games"; }
    void   onEnter() override;
    core::Signal onTick(const core::KeyEvent& ev) override;
    void   onExit() override;

private:
    core::IconMenu _menu;
    bool _menuBuilt = false;
};

}  // namespace apps
