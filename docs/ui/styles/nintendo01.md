Nintendo 2001 skins all 45 CardputerZero screens for the 240 x 135 panel as brushed metal with carbon trim and orange selection.

Derived from the getdesign.md Nintendo 2001 design file (periwinkle metal ground, carbon header, orange selection badge). Archivo Black and Arimo stand in for the original faces; Silkscreen is used for micro labels.

## Layout rules

- Header and footer are carbon-halftone bars with amber chips for status and hints. A sky breadcrumb strip under the header shows `HOME > SUB-GHZ > CAPTURE RESULT`.
- Home and menus: centred round-cornered tiles on the metal ground; the selected tile has an orange ring and an arrow badge, its caption an orange plate.
- Banners: outlined Archivo Black wordmark on a teal circuit field; red for transmit and send, lavender for empty states.
- Lists: pale rows with the selected row white and an orange arrow badge at the right.

## Colour

Use the theme `nintendo01`: `bg` #7a8aba, `tile` #dedede, `bar` #21242e, `text` #21242e, `dim` #3d4f97, `line` #3d4f97, `accent` #f68d1f, `warn` #ecab37, `sel-bg` #ffffff, `sel-fg` #21242e.

## Type

- `nintendo-title`: Arimo 8.2px, weight 700, 0.5px tracking, uppercase. Header title, uppercase.
- `nintendo-row`: Arimo 7.8px, weight 700. List rows.
- `nintendo-hint`: Arimo 6.1px, weight 700, 0.3px tracking, uppercase. Footer chips, uppercase.

## What the firmware provides

Page data only (`title`, `back`, `right`, body type, selected index, hints). The style owns every pixel of layout, icon treatment and selection; do not branch on the page in the firmware.

## Do not

- Do not use `dim` (#3d4f97) for text on `bg`; it is a line colour here.
- Do not drop the breadcrumb; it is how this style shows depth.
