// =============================================================================
// theme.cpp  -  the live theme object + selection helpers
// =============================================================================
#include "theme.h"

namespace core {

#ifndef CZ_DEFAULT_THEME
#define CZ_DEFAULT_THEME 0      // 0 = Nothing (default), 1 = Dell, 2 = Nintendo
#endif

static Theme pick(ThemeId id) {
    switch (id) {
        case ThemeId::Dell:     return THEME_DELL;
        case ThemeId::Nintendo: return THEME_NINTENDO;
        case ThemeId::Nothing:
        default:                return THEME_NOTHING;
    }
}

Theme g_theme = pick((ThemeId)CZ_DEFAULT_THEME);

void setTheme(ThemeId id) { g_theme = pick(id); }

ThemeId currentTheme() { return g_theme.id; }

ThemeId nextTheme() {
    ThemeId n = (ThemeId)(((uint8_t)g_theme.id + 1) % THEME_COUNT);
    setTheme(n);
    return n;
}

const char* themeName(ThemeId id) {
    switch (id) {
        case ThemeId::Dell:     return "Dell 1996";
        case ThemeId::Nintendo: return "Nintendo 2001";
        case ThemeId::Nothing:
        default:                return "Nothing OS";
    }
}

}  // namespace core
