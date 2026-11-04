Nothing OS skins all 45 CardputerZero screens for the 240 x 135 panel as dot-matrix type on black with one red signal.

Colours come from the firmware's own `core/ui.h`; the language is Nothing's public design (dot-matrix display type, monochrome, red accent). Nothing OS is not in getdesign.md, so this style is hand-built.

## Layout rules

- Home and menus: a centred, wrapping grid of round tiles with dotted glyphs. Idle tiles ring in `line`; the selected tile rings in `accent` and its caption turns `accent`.
- Lists: the selected row is a full-width `sel-bg` pill with `sel-fg` text; other rows are `text` on `bg`.
- Banners: a dotted glyph in concentric rings on the left, one sentence in `nothing-row` on the right. Use `warn` for transmit and send states.
- Header title in `nothing-title` (Doto 900); everything else in Space Mono. Footer: key chips, then hints in `dim`.

## Colour

Use the theme `nothing`: `bg` #000000, `tile` #161618, `bar` #0B0B0C, `text` #F2F2F2, `dim` #6A6A70, `line` #2A2A2E, `accent` #D7171E, `warn` #E6B42C, `sel-bg` #D7171E, `sel-fg` #000000.

## Type

- `nothing-title`: Doto 12.5px, weight 900, 0.3px tracking. Page title in the header bar.
- `nothing-row`: Space Mono 8.2px, weight 400. List rows; 700 when selected.
- `nothing-hint`: Space Mono 6.6px, weight 400, 0.2px tracking. Footer key hints and captions.

## What the firmware provides

Page data only (`title`, `back`, `right`, body type, selected index, hints). The style owns every pixel of layout, icon treatment and selection; do not branch on the page in the firmware.

## Do not

- Do not fill panels with `accent`; red marks selection and live state only.
- Do not set Doto below 10 px; the dots merge.
- Do not outline icons: draw them dotted.
