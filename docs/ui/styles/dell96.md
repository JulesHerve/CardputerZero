Dell 1996 skins all 45 CardputerZero screens for the 240 x 135 panel as an early-web catalogue with a black frame, ribbon cards and blue links.

Derived from the getdesign.md Dell 1996 design file (hard black frame, red calls to action, tinted ribbon cards, default blue links). Arimo stands in for Helvetica and Tinos for Times; list rows, key-value rows and footer hints are set in Azeret Mono.

## Layout rules

- A 3 px black frame surrounds every page; the header is a black banner with the title in `dell-title` and the status in red (`accent`).
- Home and menus: ribbon cards with a black label tab and a tinted body (peach, sage, salmon, sky, periwinkle, lime). The selected card gets a thick black outline and a yellow `NEW!` sticker.
- Lists: Azeret Mono rows (`dell-row`) with square bullets; the selected row is a sage bar with a bold blue underlined link. Key-value rows use the same face.
- Banners are flat blocks: red for ordinary states, yellow (`warn`) for transmit and send, sage (`tile`) for dim or empty. The message is Tinos italic (`dell-copy`).
- Footer hints are blue underlined links in Azeret Mono (`dell-hint`) with the key in a bold bracket, `[esc] back`; the home sub-line is plain mono.

## Colour

Use the theme `dell96`: `bg` #ffffff, `tile` #b3bd95, `bar` #000000, `text` #000000, `dim` #5a5a5a, `line` #000000, `accent` #e91d2a, `warn` #fcc20f, `sel-bg` #000000, `sel-fg` #ffffff.

## Type

- `dell-title`: Arimo 8.6px, weight 700, 0.2px tracking, uppercase. Header banner title, uppercase.
- `dell-row`: Azeret Mono 8.2px, weight 400. List rows and key-value rows; 700 and underlined when selected.
- `dell-hint`: Azeret Mono 6.6px, weight 400. Footer link hints and sub-lines; key brackets in 700 at 6.2 px.
- `dell-display`: Archivo Black 12px, weight 400. Tag headings, edge counts and the splash wordmark; 11-15 px in the mockups.
- `dell-copy`: Tinos 10.4px, weight 400. Banner and confirm messages, set in italic (confirm in 700).

## What the firmware provides

Page data only (`title`, `back`, `right`, body type, selected index, hints). The style owns every pixel of layout, icon treatment and selection; do not branch on the page in the firmware.

## Do not

- Do not round corners or add gradients.
- Do not set rows or hints in a proportional serif; Azeret Mono is the face for both.
- Do not use `dim` (#5a5a5a) on the sage `tile`; it misses 4.5:1.
