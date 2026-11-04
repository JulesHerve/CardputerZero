// =============================================================================
// games.cpp  -  Snake, Tetris, Flappy, and a Doom-style raycaster
//
// Each game runs its own loop and draws into an off-screen M5Canvas sprite that
// is pushed whole each frame (no flicker). Keys are read straight from the
// matrix every frame via readPad() so holding a key repeats.
// =============================================================================
#include "games.h"
#include "../core/input.h"
#include <M5Cardputer.h>
#include <math.h>

using namespace core;

namespace {

constexpr int SW = 240, SH = 135;

inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return M5Cardputer.Display.color565(r, g, b); }

// ---- per-frame key state (held keys repeat) --------------------------------
struct Pad { bool up = false, down = false, left = false, right = false, ok = false, esc = false; };

Pad readPad() {
    M5Cardputer.update();
    Pad p;
    auto st = M5Cardputer.Keyboard.keysState();
    if (st.enter) p.ok = true;
    for (char c : st.word) {
        switch (c) {
            case ';': p.up = true; break;
            case '.': p.down = true; break;
            case ',': p.left = true; break;
            case '/': p.right = true; break;
            case '`': p.esc = true; break;
            default: break;
        }
    }
    return p;
}

void beep(int f, int ms) { M5Cardputer.Speaker.tone(f, ms); }

// Draw a GAME OVER card and wait. Returns true to retry, false to quit.
bool gameOver(M5Canvas& cv, const char* name, int score) {
    cv.fillSprite(rgb(0, 0, 0));
    cv.setTextDatum(middle_center);
    cv.setFont(&fonts::Font4);
    cv.setTextColor(rgb(0xD7, 0x17, 0x1E));
    cv.drawString("GAME OVER", SW / 2, 40);
    cv.setFont(&fonts::Font2);
    cv.setTextColor(rgb(0xF2, 0xF2, 0xF2));
    char b[40]; snprintf(b, sizeof(b), "%s   score %d", name, score);
    cv.drawString(b, SW / 2, 72);
    cv.setFont(&fonts::Font0);
    cv.setTextColor(rgb(0x9A, 0x9A, 0xA0));
    cv.drawString("[ENTER] retry     [`] quit", SW / 2, 100);
    cv.pushSprite(0, 0);
    delay(250);                                   // debounce the death key
    for (;;) {
        Pad p = readPad();
        if (p.ok)  { delay(120); return true; }
        if (p.esc) { delay(120); return false; }
        delay(10);
    }
}

// =============================================================================
// SNAKE
// =============================================================================
void playSnake() {
    M5Canvas cv(&M5Cardputer.Display);
    if (!cv.createSprite(SW, SH)) return;

    const int cell = 6;
    const int gx0 = 0, gy0 = 12;
    const int cols = SW / cell;                   // 40
    const int rows = (SH - gy0) / cell;           // 20
    const int MAXN = 40 * 20;

    do {
        static int sx[800], sy[800];
        int len = 4, dx = 1, dy = 0;
        sx[0] = cols / 2; sy[0] = rows / 2;
        for (int i = 1; i < len; ++i) { sx[i] = sx[0] - i; sy[i] = sy[0]; }
        auto placeFood = [&](int& fx, int& fy) {
            for (;;) {
                fx = random(cols); fy = random(rows);
                bool on = false;
                for (int i = 0; i < len; ++i) if (sx[i] == fx && sy[i] == fy) { on = true; break; }
                if (!on) return;
            }
        };
        int fx, fy; placeFood(fx, fy);
        int score = 0;
        uint32_t last = millis();
        int stepMs = 120;
        bool dead = false, quit = false;

        while (!dead) {
            Pad p = readPad();
            if (p.esc) { quit = true; break; }
            if (p.up    && dy == 0) { dx = 0; dy = -1; }
            else if (p.down  && dy == 0) { dx = 0; dy = 1; }
            else if (p.left  && dx == 0) { dx = -1; dy = 0; }
            else if (p.right && dx == 0) { dx = 1; dy = 0; }

            if (millis() - last < (uint32_t)stepMs) { delay(4); continue; }
            last = millis();

            int nx = sx[0] + dx, ny = sy[0] + dy;
            if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) { dead = true; }
            else {
                for (int i = 0; i < len; ++i) if (sx[i] == nx && sy[i] == ny) { dead = true; break; }
            }
            if (dead) { beep(200, 120); break; }

            bool ate = (nx == fx && ny == fy);
            for (int i = (ate ? len : len - 1); i > 0; --i) { sx[i] = sx[i - 1]; sy[i] = sy[i - 1]; }
            sx[0] = nx; sy[0] = ny;
            if (ate) {
                if (len < MAXN) len++;
                score += 10; beep(1500, 15);
                if (stepMs > 55) stepMs -= 3;
                placeFood(fx, fy);
            }

            // render
            cv.fillSprite(rgb(0, 0, 0));
            cv.drawRect(0, gy0, SW, rows * cell, rgb(0x2A, 0x2A, 0x2E));
            cv.fillRect(gx0 + fx * cell, gy0 + fy * cell, cell - 1, cell - 1, rgb(0xD7, 0x17, 0x1E));
            for (int i = 0; i < len; ++i)
                cv.fillRect(gx0 + sx[i] * cell, gy0 + sy[i] * cell, cell - 1, cell - 1,
                            i == 0 ? rgb(0x8B, 0xE3, 0x4A) : rgb(0x4C, 0xAF, 0x50));
            cv.setFont(&fonts::Font0); cv.setTextDatum(top_left);
            cv.setTextColor(rgb(0xF2, 0xF2, 0xF2));
            char h[24]; snprintf(h, sizeof(h), "SNAKE  %d", score);
            cv.drawString(h, 4, 2);
            cv.pushSprite(0, 0);
        }
        if (quit) break;
        if (!gameOver(cv, "SNAKE", score)) break;
    } while (true);

    cv.deleteSprite();
}

// =============================================================================
// TETRIS
// =============================================================================
void playTetris() {
    M5Canvas cv(&M5Cardputer.Display);
    if (!cv.createSprite(SW, SH)) return;

    const int BW = 10, BH = 20, cell = 6;
    const int bx0 = (SW - BW * cell) / 2, by0 = (SH - BH * cell) / 2 + 2;

    // 7 tetromino base cells in a 4x4 box.
    static const int8_t P[7][4][2] = {
        {{0,1},{1,1},{2,1},{3,1}},  // I
        {{1,0},{2,0},{1,1},{2,1}},  // O
        {{1,0},{0,1},{1,1},{2,1}},  // T
        {{1,0},{2,0},{0,1},{1,1}},  // S
        {{0,0},{1,0},{1,1},{2,1}},  // Z
        {{0,0},{0,1},{1,1},{2,1}},  // J
        {{2,0},{0,1},{1,1},{2,1}},  // L
    };
    const uint16_t COL[7] = {
        rgb(0x2E,0xC4,0xE0), rgb(0xEC,0xD6,0x1E), rgb(0xA8,0x4C,0xD0),
        rgb(0x4C,0xAF,0x50), rgb(0xD7,0x3A,0x3A), rgb(0x3A,0x6C,0xE0), rgb(0xF0,0x8A,0x24),
    };

    auto cellOf = [&](int type, int rot, int i, int& ox, int& oy) {
        int x = P[type][i][0], y = P[type][i][1];
        for (int r = 0; r < (rot & 3); ++r) { int nx = 3 - y, ny = x; x = nx; y = ny; }
        ox = x; oy = y;
    };

    do {
        static uint8_t grid[BH][BW];
        for (int y = 0; y < BH; ++y) for (int x = 0; x < BW; ++x) grid[y][x] = 0;

        int type = random(7), rot = 0, px = 3, py = 0, score = 0, lines = 0;
        auto collide = [&](int t, int r, int ppx, int ppy) {
            for (int i = 0; i < 4; ++i) {
                int ox, oy; cellOf(t, r, i, ox, oy);
                int X = ppx + ox, Y = ppy + oy;
                if (X < 0 || X >= BW || Y >= BH) return true;
                if (Y >= 0 && grid[Y][X]) return true;
            }
            return false;
        };
        bool dead = false, quit = false;
        uint32_t lastDrop = millis();
        int dropMs = 500, frame = 0, moveCd = 0;
        bool prevUp = false;

        while (!dead) {
            Pad p = readPad();
            if (p.esc) { quit = true; break; }
            if (p.up && !prevUp) { if (!collide(type, rot + 1, px, py)) { rot = (rot + 1) & 3; beep(1200, 8); } }
            prevUp = p.up;
            if (moveCd > 0) moveCd--;
            if ((p.left || p.right) && moveCd == 0) {
                int nx = px + (p.right ? 1 : -1);
                if (!collide(type, rot, nx, py)) px = nx;
                moveCd = 4;
            }
            bool soft = p.down;

            if (millis() - lastDrop > (uint32_t)(soft ? 45 : dropMs)) {
                lastDrop = millis();
                if (!collide(type, rot, px, py + 1)) py++;
                else {
                    for (int i = 0; i < 4; ++i) { int ox, oy; cellOf(type, rot, i, ox, oy);
                        int X = px + ox, Y = py + oy; if (Y >= 0 && Y < BH && X >= 0 && X < BW) grid[Y][X] = type + 1; }
                    int cleared = 0;
                    for (int y = BH - 1; y >= 0; --y) {
                        bool full = true;
                        for (int x = 0; x < BW; ++x) if (!grid[y][x]) { full = false; break; }
                        if (full) {
                            cleared++;
                            for (int yy = y; yy > 0; --yy) for (int x = 0; x < BW; ++x) grid[yy][x] = grid[yy - 1][x];
                            for (int x = 0; x < BW; ++x) grid[0][x] = 0;
                            y++;
                        }
                    }
                    if (cleared) { score += (cleared == 1 ? 100 : cleared == 2 ? 300 : cleared == 3 ? 500 : 800);
                                   lines += cleared; dropMs = 500 - (lines / 10) * 40; if (dropMs < 120) dropMs = 120;
                                   beep(1800, 40); }
                    type = random(7); rot = 0; px = 3; py = 0;
                    if (collide(type, rot, px, py)) { dead = true; beep(200, 160); break; }
                }
            }

            // render
            cv.fillSprite(rgb(0, 0, 0));
            cv.drawRect(bx0 - 2, by0 - 2, BW * cell + 4, BH * cell + 4, rgb(0x55, 0x55, 0x60));
            for (int y = 0; y < BH; ++y) for (int x = 0; x < BW; ++x) if (grid[y][x])
                cv.fillRect(bx0 + x * cell, by0 + y * cell, cell - 1, cell - 1, COL[grid[y][x] - 1]);
            for (int i = 0; i < 4; ++i) { int ox, oy; cellOf(type, rot, i, ox, oy);
                int X = px + ox, Y = py + oy; if (Y >= 0)
                    cv.fillRect(bx0 + X * cell, by0 + Y * cell, cell - 1, cell - 1, COL[type]); }
            cv.setFont(&fonts::Font0); cv.setTextDatum(top_left);
            cv.setTextColor(rgb(0xF2, 0xF2, 0xF2));
            char h[20]; snprintf(h, sizeof(h), "LINES %d", lines); cv.drawString(h, 4, 14);
            snprintf(h, sizeof(h), "%d", score); cv.drawString(h, 4, 28);
            cv.setTextColor(rgb(0x9A, 0x9A, 0xA0));
            cv.drawString("; rot", 4, 60); cv.drawString(", / mv", 4, 72); cv.drawString(". drop", 4, 84);
            cv.pushSprite(0, 0);
            frame++;
        }
        if (quit) break;
        if (!gameOver(cv, "TETRIS", score)) break;
    } while (true);

    cv.deleteSprite();
}

// =============================================================================
// FLAPPY
// =============================================================================
void playFlappy() {
    M5Canvas cv(&M5Cardputer.Display);
    if (!cv.createSprite(SW, SH)) return;

    const int birdX = 64, gap = 46, pipeW = 22, ground = SH - 8;
    const float grav = 0.42f, flap = -4.4f;

    do {
        float by = SH / 2.0f, vy = 0;
        struct Pipe { int x; int gapY; bool scored; } pipes[3];
        for (int i = 0; i < 3; ++i) pipes[i] = { SW + i * 90, 30 + random(SH - 60 - gap), false };
        int score = 0; bool dead = false, quit = false, prevFlap = false;

        while (!dead) {
            Pad p = readPad();
            if (p.esc) { quit = true; break; }
            bool flapNow = p.ok || p.up;
            if (flapNow && !prevFlap) { vy = flap; beep(900, 8); }
            prevFlap = flapNow;

            vy += grav; by += vy;
            if (by < 6) { by = 6; vy = 0; }
            if (by > ground - 6) { dead = true; }

            for (int i = 0; i < 3; ++i) {
                pipes[i].x -= 2;
                if (pipes[i].x < -pipeW) {
                    int mx = pipes[0].x; for (int j = 1; j < 3; ++j) if (pipes[j].x > mx) mx = pipes[j].x;
                    pipes[i].x = mx + 90; pipes[i].gapY = 26 + random(SH - 52 - gap); pipes[i].scored = false;
                }
                if (!pipes[i].scored && pipes[i].x + pipeW < birdX) { pipes[i].scored = true; score++; beep(1600, 15); }
                // collision
                if (birdX + 6 > pipes[i].x && birdX - 6 < pipes[i].x + pipeW) {
                    if (by - 6 < pipes[i].gapY || by + 6 > pipes[i].gapY + gap) dead = true;
                }
            }
            if (dead) beep(200, 140);

            // render
            cv.fillSprite(rgb(0x3C, 0x8F, 0xC9));                 // sky
            for (int i = 0; i < 3; ++i) {
                cv.fillRect(pipes[i].x, 0, pipeW, pipes[i].gapY, rgb(0x3F, 0xA0, 0x3F));
                cv.fillRect(pipes[i].x, pipes[i].gapY + gap, pipeW, SH - (pipes[i].gapY + gap), rgb(0x3F, 0xA0, 0x3F));
            }
            cv.fillRect(0, ground, SW, SH - ground, rgb(0x7A, 0x5A, 0x2A)); // ground
            cv.fillCircle(birdX, (int)by, 6, rgb(0xF4, 0xD0, 0x30));        // bird
            cv.fillCircle(birdX + 2, (int)by - 2, 2, rgb(0, 0, 0));
            cv.setFont(&fonts::Font2); cv.setTextDatum(top_center);
            cv.setTextColor(rgb(0xFF, 0xFF, 0xFF));
            char h[12]; snprintf(h, sizeof(h), "%d", score); cv.drawString(h, SW / 2, 4);
            cv.pushSprite(0, 0);
            delay(22);
        }
        if (quit) break;
        if (!gameOver(cv, "FLAPPY", score)) break;
    } while (true);

    cv.deleteSprite();
}

// =============================================================================
// DOOM-style raycaster
// =============================================================================
void playDoom() {
    M5Canvas cv(&M5Cardputer.Display);
    if (!cv.createSprite(SW, SH)) return;

    static const char* MAP[16] = {
        "################",
        "#..............#",
        "#..####..###...#",
        "#..#.......#...#",
        "#..#..##...#...#",
        "#.....#....#...#",
        "#..####....#####",
        "#..............#",
        "#..###..####...#",
        "#....#.....#...#",
        "#....#.....#...#",
        "#..###..####...#",
        "#..............#",
        "#....######....#",
        "#..............#",
        "################",
    };
    auto wall = [&](int x, int y) { return (x < 0 || y < 0 || x >= 16 || y >= 16) || MAP[y][x] == '#'; };

    float px = 3.5f, py = 3.5f, ang = 0.0f;
    const float FOV = 1.05f;                 // ~60 deg
    const int horizon = SH / 2;
    bool quit = false;

    while (!quit) {
        Pad p = readPad();
        if (p.esc) break;
        if (p.left)  ang -= 0.09f;
        if (p.right) ang += 0.09f;
        float mv = p.up ? 0.09f : (p.down ? -0.09f : 0.0f);
        if (mv != 0.0f) {
            float nx = px + cosf(ang) * mv, ny = py + sinf(ang) * mv;
            if (!wall((int)nx, (int)py)) px = nx;
            if (!wall((int)px, (int)ny)) py = ny;
        }

        cv.fillRect(0, 0, SW, horizon, rgb(0x20, 0x22, 0x2C));        // ceiling
        cv.fillRect(0, horizon, SW, SH - horizon, rgb(0x4A, 0x38, 0x28)); // floor

        const int step = 2;
        for (int x = 0; x < SW; x += step) {
            float rayA = ang - FOV / 2 + FOV * ((float)x / SW);
            float rdx = cosf(rayA), rdy = sinf(rayA);
            int mapX = (int)px, mapY = (int)py;
            float dX = (rdx == 0) ? 1e30f : fabsf(1.0f / rdx);
            float dY = (rdy == 0) ? 1e30f : fabsf(1.0f / rdy);
            int stepX, stepY; float sideX, sideY;
            if (rdx < 0) { stepX = -1; sideX = (px - mapX) * dX; } else { stepX = 1; sideX = (mapX + 1 - px) * dX; }
            if (rdy < 0) { stepY = -1; sideY = (py - mapY) * dY; } else { stepY = 1; sideY = (mapY + 1 - py) * dY; }
            int side = 0, guard = 0; bool hit = false;
            while (!hit && guard++ < 64) {
                if (sideX < sideY) { sideX += dX; mapX += stepX; side = 0; }
                else               { sideY += dY; mapY += stepY; side = 1; }
                if (mapX < 0 || mapY < 0 || mapX >= 16 || mapY >= 16) { hit = true; break; }
                if (MAP[mapY][mapX] == '#') hit = true;
            }
            float dist = (side == 0) ? (mapX - px + (1 - stepX) / 2.0f) / (rdx == 0 ? 1e-3f : rdx)
                                     : (mapY - py + (1 - stepY) / 2.0f) / (rdy == 0 ? 1e-3f : rdy);
            dist *= cosf(rayA - ang);                 // de-fisheye
            if (dist < 0.05f) dist = 0.05f;
            int lineH = (int)(SH / dist);
            int d0 = horizon - lineH / 2, d1 = horizon + lineH / 2;
            if (d0 < 0) d0 = 0; if (d1 > SH) d1 = SH;
            int sh = 235 - (int)(dist * 26); if (sh < 45) sh = 45; if (sh > 235) sh = 235;
            if (side == 1) sh = sh * 2 / 3;
            cv.fillRect(x, d0, step, d1 - d0, rgb(sh, (uint8_t)(sh * 0.7f), (uint8_t)(sh * 0.4f)));
        }

        // simple gun + HUD
        cv.fillRect(SW / 2 - 8, SH - 20, 16, 20, rgb(0x30, 0x30, 0x34));
        cv.fillRect(SW / 2 - 3, SH - 34, 6, 16, rgb(0x20, 0x20, 0x24));
        cv.setFont(&fonts::Font0); cv.setTextDatum(top_left);
        cv.setTextColor(rgb(0xD7, 0x17, 0x1E));
        cv.drawString("DOOM  ;/. move  ,// turn  ` quit", 4, 2);
        cv.pushSprite(0, 0);
        delay(8);
    }
    cv.deleteSprite();
}

}  // namespace

namespace apps {

void Games::onEnter() {
    randomSeed(micros());
    if (!_menuBuilt) {
        _menu.add(core::Icon::Games, "SNAKE",  "Classic snake");
        _menu.add(core::Icon::Games, "TETRIS", "Stack the blocks");
        _menu.add(core::Icon::Games, "FLAPPY", "Tap to flap");
        _menu.add(core::Icon::Games, "DOOM",   "Raycaster FPS demo");
        _menuBuilt = true;
    }
    _menu.draw("Games");
}

void Games::onExit() {}

core::Signal Games::onTick(const core::KeyEvent& ev) {
    if (ev.key == Key::Left)  { _menu.move(-1, 0); _menu.draw("Games"); }
    if (ev.key == Key::Right) { _menu.move(+1, 0); _menu.draw("Games"); }
    if (ev.key == Key::Up)    { _menu.move(0, -1); _menu.draw("Games"); }
    if (ev.key == Key::Down)  { _menu.move(0, +1); _menu.draw("Games"); }
    if (ev.key == Key::Back) return Signal::Back;
    if (ev.key == Key::Ok) {
        switch (_menu.selection()) {
            case 0: playSnake();  break;
            case 1: playTetris(); break;
            case 2: playFlappy(); break;
            case 3: playDoom();   break;
        }
        _menu.draw("Games");           // redraw the menu after the game returns
    }
    return Signal::Stay;
}

}  // namespace apps
