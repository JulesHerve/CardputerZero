# UI themes: what the firmware implements

CardputerZero ships three selectable skins. **Nothing OS is the default and is
unchanged** from the original firmware; Dell 1996 and Nintendo 2001 are new.

## How to switch themes

- **On the device:** from the home screen press **`t`** to cycle
  Nothing → Dell → Nintendo. The choice is saved to `/czero/theme.txt` on the SD
  card and restored on the next boot.
- **At build time:** pass `-DCZ_DEFAULT_THEME=0|1|2` (0 Nothing, 1 Dell,
  2 Nintendo) to set the power-on default when there is no SD card / saved value.

## Where the code lives

| file | role |
|---|---|
| `src/core/theme.h` | `Theme` struct + the three palettes (exact hex from `styles/*.md`, packed to RGB565) and `setTheme` / `nextTheme` / `currentTheme`. |
| `src/core/theme.cpp` | the live `g_theme` object + selection helpers. |
| `src/core/ui.h` | every colour role is read from `g_theme`; `header`/`footer`/`list`/banners branch on `Theme::id` for per-style selection shape and hint styling. |
| `src/core/iconmenu.h` | per-style tile treatment (Nothing red ring, Dell sage ribbon + NEW! tag, Nintendo orange plate). |
| `src/main.cpp` | loads/saves the theme and the `t` toggle. |

Colour **roles** carry three extra entries beyond the design docs so all three
palettes stay legible on the fixed hardware: `barText` (text drawn on the header
bar — needed because Dell's bar is black and Nintendo's is carbon), plus `selBg`
/ `selFg` for the selected row. The Nothing palette's RGB565 values are
byte-identical to the firmware's original hard-coded colours, so its look does
not change.

## Fonts — which M5GFX built-ins were chosen

The mockups use web fonts (Doto, Space Mono, Arimo, Archivo Black, Azeret Mono,
Silkscreen, Tinos) that are **not on the device**. Converting seven TTFs to
bitmap fonts would add significant flash and cannot be verified without a
hardware build, so each face is mapped to an M5GFX built-in:

| mockup face | used for | mapped to |
|---|---|---|
| Doto 900 / Arimo 700 | header titles | `fonts::Font2` (uppercased for Dell/Nintendo) |
| Space Mono / Azeret Mono / Arimo | list & key-value rows | `fonts::Font2` |
| Tinos | banner & confirm copy | `fonts::Font2` |
| Space Mono / Azeret Mono / Arimo | footer hints & captions | `fonts::Font0` |
| Archivo Black | tag headings / wordmark | `fonts::Font2` |

(Per the Dell brief, row and hint text is **not** set in a serif — Azeret Mono's
role maps to the mono-ish built-in, not a Tinos stand-in.)

## Faithful vs. approximated

Implemented per style: palette, header/footer colour treatment, selected-row
shape (Nothing rounded red pill, Dell square black bar + square bullets,
Nintendo white row + orange arrow), icon-tile treatment (Dell sage ribbon with a
yellow **NEW!** sticker on the selected card; Nintendo light tile with an orange
caption plate), Dell blue underlined "link" footer hints, Nintendo amber
uppercase hints.

Approximated because the hardware fonts and the fixed 18/103/14 px layout do not
allow a pixel copy: the exact web typefaces, the Nintendo breadcrumb strip
(`HOME > … > …` — the firmware only passes a single page title), and the finer
halftone/metal textures. These are colour/role faithful but not pixel-faithful.
