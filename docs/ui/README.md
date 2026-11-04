# CardputerZero UI: target designs

Three UI styles for the 240 x 135 px Cardputer ADV screen, 45 screens each. `nothing` is today's look (the palette in `src/core/ui.h`); `dell96` and `nintendo01` are new.

## What is here

| path | what |
|---|---|
| `screens/<style>/NN-<id>.png` | every page, 960 x 540 (the 240 x 135 UI at 4x). The visual source of truth. |
| `contact-sheets/<style>.png` | all 45 pages of one style on one sheet |
| `device-mockups/<style>-home.png` | home screen on an illustrative body (not to scale) |
| `styles/<style>.md` | layout, colour, type and do-not rules per style |
| `firmware/cz_themes.h` | RGB565 colour roles for the three styles |
| `firmware/palettes.json` | same in hex, plus fonts |
| `reference/mockup-engine/` | the HTML/CSS that drew the PNGs; read `themes/<style>.js` for exact pixel metrics (not firmware) |

## Firmware map (what to change)

- `src/core/ui.h`: palette (`cBg`, `cTile`, `cBar`, `cText`, `cDim`, `cLine`, `cAccent`, `cWarn`), `header`, `footer`, `list`, banners, console. Almost every app screen goes through `UI::`.
- `src/core/iconmenu.h` + `icons.h`: home screen and in-app menus (3-column icon grid, up to 6 items).
- `src/apps/*.cpp`: apps call `UI::` helpers; they draw nothing directly, so keep them unchanged where you can.
- `src/main.cpp`: home screen and splash.

## Rules

- Screen is 240 x 135, landscape, no touch. Header 18 px, footer 14 px, body 103 px.
- Keys and behaviour do not change: `;` `.` move, `enter` selects, `` ` `` / `esc` goes back, `s` save, `r` rescan.
- Keep Nothing OS as the default. Add the others as selectable themes (compile flag first, a menu setting later).
- Fonts in the mockups are web fonts (Doto, Space Mono, Arimo, Archivo Black, Azeret Mono, Silkscreen, Tinos) and are not on the device. Map each to an M5GFX built-in font, or convert the TTF to a bitmap font and check flash use. Say which you chose.
- Some mockup pages show data the current screens do not draw as a view (Sub-GHz capture result, NFC tag identity, AP detail, bar charts). Add `UI::` helpers for them and keep the app change small.
- Build with `pio run -e cardputer-adv`. Do not flash or upload without asking.

## Suggested order

1. Make the palette runtime-selectable from `cz_themes.h` without changing the look of `nothing`.
2. `header` / `footer` per style.
3. `list` per style.
4. Banners and console.
5. `IconMenu` (home and menus) per style.
6. The data views: result, hbars, vbars, tag, ndef, kv, confirm, splash.

After each step, build, then compare the screen to the matching PNG. Claude Code cannot see the device: photograph the screen and paste the photo to compare.

## Pages

| file | app | screen | body type |
|---|---|---|---|
| `00-splash` | system | Splash | splash |
| `01-home` | system | Home | home |
| `02-subghz-menu` | subghz | Sub-GHz menu | menu |
| `03-subghz-frequency` | subghz | Set frequency | list |
| `04-subghz-receiving` | subghz | Receiving | banner |
| `05-subghz-capture` | subghz | Capture result | result |
| `06-subghz-capture-empty` | subghz | Nothing captured | banner |
| `07-subghz-transmitting` | subghz | Transmitting | banner |
| `08-subghz-analyzer` | subghz | Analyzer | hbars |
| `09-subghz-load` | subghz | Load capture | list |
| `10-subghz-load-empty` | subghz | No SD card | banner |
| `11-subghz-saved` | subghz | Saved to SD | banner |
| `12-nfc-menu` | nfc | NFC menu | menu |
| `13-nfc-read` | nfc | Read: hold a tag | banner |
| `14-nfc-tag` | nfc | Tag identity | tag |
| `15-nfc-content` | nfc | NFC content | ndef |
| `16-nfc-content-image` | nfc | NFC content (image) | ndef |
| `17-nfc-content-none` | nfc | No NDEF | banner |
| `18-nfc-emulate-wait` | nfc | Emulate: waiting | banner |
| `19-nfc-emulate-active` | nfc | Emulate: active | banner |
| `20-nfc-serial-dumping` | nfc | Serial dump: running | banner |
| `21-nfc-serial-done` | nfc | Serial dump: done | banner |
| `22-nfc-saved` | nfc | Saved to CSV | banner |
| `23-ir-menu` | ir | Infrared menu | menu |
| `24-ir-learn` | ir | Learn: point remote | banner |
| `25-ir-learned` | ir | Learned a button | banner |
| `26-ir-send` | ir | Replay: sending | banner |
| `27-ir-tv-sweep` | ir | Universal TV power | banner |
| `28-ir-load` | ir | Load IR button | list |
| `29-wifi-menu` | wifi | Wi-Fi menu | menu |
| `30-wifi-scanning` | wifi | Scanning | banner |
| `31-wifi-list` | wifi | AP list | list |
| `32-wifi-channels` | wifi | Channel graph | vbars |
| `33-wifi-detail` | wifi | AP detail | kv |
| `34-ble-menu` | ble | BLE menu | menu |
| `35-ble-scanning` | ble | Scanning 5s | banner |
| `36-ble-list` | ble | BLE results | list |
| `37-ble-pairing` | ble | Keyboard: pairing | banner |
| `38-ble-linked` | ble | Keyboard: linked | banner |
| `39-usb-menu` | usb | USB menu | menu |
| `40-usb-macros` | usb | Macro list | list |
| `41-usb-confirm` | usb | Confirm run | confirm |
| `42-usb-running` | usb | Macro running | console |
| `43-usb-done` | usb | Macro finished | banner |
| `44-usb-keyboard` | usb | USB keyboard | banner |
