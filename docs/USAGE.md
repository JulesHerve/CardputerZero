# Using CardputerZero

Boot the device and you land on the **home screen** — a grid of app icons
(Nothing-OS styled). Move around the grid with the arrow keys (or `;` `.` `,`
`/`), and press **Enter** to open the highlighted tool. The line at the bottom
describes the selected app. Navigation everywhere:

- **`;` / `.`** or **↑ / ↓** — move
- **Enter** — select / act
- **`` ` ``** or **Esc** — back
- Letter keys = the shortcuts shown in each screen's footer.

Everything saved goes under `/czero/…` on the microSD card.

---

## Sub-GHz (CC1101)

For your own 315 / 433 / 868–915 MHz remotes and sensors (garage/gate remotes,
weather stations, doorbells you own).

1. **Set frequency** — pick the band your remote uses (check its label; most
   garage/gate remotes in the EU are 433.92 MHz).
2. **Receive (raw OOK)** — the device listens. Press your remote near it. When
   the burst ends you'll see how many edges were captured.
3. From the result screen: **Enter** replays it, **`s`** saves it to
   `/czero/subghz/` as a `.sub` text file.
4. **Replay last capture** re-sends whatever is in memory.
5. **Load from SD** brings a saved capture back so you can replay it.
6. **Frequency analyzer** sweeps RSSI across the presets — a quick way to see
   which band something is transmitting on (watch the bar jump when you press a
   remote).

> The capture is raw timing (OOK), which covers the vast majority of simple
> fixed-code remotes. It does **not** defeat rolling-code security (e.g. KeeLoq)
> — replaying a rolling-code remote won't work, by design.

## NFC / RFID (ST25R3916)

For your own 13.56 MHz tags — your **NTAG203** and **MIFARE Classic 1K** are
both supported. The tool opens as an icon sub-menu:

- **READ** — hold a tag to the **back of the Cap**; shows type, UID, ATQA, SAK.
  Press **`s`** on the result to log it to `/czero/nfc/tags.csv`.
- **CONTENT** — decodes the tag's **NDEF** message and shows the actual content,
  not just the UID: **text** records, **links/URLs**, and small **images**
  (`image/png` records are drawn on screen). Write an NDEF text/URL/image to your
  NTAG from a phone app, then read it here.
- **SERIAL** — full memory dump over **USB serial** (open the serial monitor).
  MIFARE Classic uses the default key `FF FF FF FF FF FF`; NTAG ignores keys.

## Infrared

For your own TVs, projectors, air-con, etc. Icon sub-menu:

- **LEARN** — point a remote at the **Mini IR Unit** (Grove) and press a key; it
  stores the decoded protocol, or raw timing if the protocol is unknown.
- **REPLAY** — re-sends the learned button through the Mini IR Unit's emitter.
- **TV PWR** — fires common Sony / RC5 / Samsung / LG / NEC power codes in turn.
  Handy for your own TV; not every set responds.
- **LOAD** — restore a saved `.ir`. Press **`s`** to save the current button.

> IR now transmits **and** learns through the Mini IR Unit on the Grove port
> (emitter G2, receiver G1). To emit through the Cardputer's built-in LED
> instead, set `PIN_IR_TX` back to `PIN_IR_TX_BUILTIN` in `src/config.h`.

## Wi-Fi scan

Lists nearby access points (SSID, signal, channel, security). **Passive only**
— it never connects or interferes. **Enter** rescans.

## BLE

Icon sub-menu:

- **SCAN** — passive list of nearby BLE advertisers (name, signal, address).
  Listen-only; **Enter** rescans.
- **KEYBOARD** — the Cardputer becomes a **BLE HID keyboard**. Open it, then on
  your own PC / phone / tablet pair the device named **"CardputerZero"** from the
  Bluetooth menu. Once it says *linked*, whatever you type on the Cardputer is
  sent to that device over Bluetooth (arrow keys and Enter included). It's the
  wireless sibling of the USB macro tool. Press **`` ` ``** to disconnect.

## USB macro (HID)

Runs a small keyboard script on **the computer the Cardputer is plugged into** —
for automating your own machine.

1. Put `.txt` scripts in `/czero/badusb/` on the SD card.
2. Pick one; a confirmation screen reminds you it types into this computer.
3. **Enter** confirms and runs it. **`` ` ``** aborts mid-run.

Script commands (one per line):

```
REM this is a comment
DEFAULTDELAY 50
DELAY 500
STRING hello world
STRINGLN this line presses enter after
GUI r              REM Win+R
CTRL c             REM Ctrl+C
ALT TAB
CTRL-ALT t         REM a terminal on many Linux desktops
ENTER / TAB / ESC / SPACE / UP / DOWN / LEFT / RIGHT
```

See `data/czero/badusb/hello.txt` for a working example.
