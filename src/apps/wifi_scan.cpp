// =============================================================================
// wifi_scan.cpp  -  Wi-Fi tool: passive scanning + your-own-AP features
// =============================================================================
#include "wifi_scan.h"
#include "../core/input.h"
#include "../core/storage.h"
#include "../config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <vector>
#include <string>

using namespace core;

namespace {
const char* encStr(wifi_auth_mode_t m) {
    switch (m) {
        case WIFI_AUTH_OPEN:         return "OPEN";
        case WIFI_AUTH_WEP:          return "WEP";
        case WIFI_AUTH_WPA_PSK:      return "WPA";
        case WIFI_AUTH_WPA2_PSK:     return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
        case WIFI_AUTH_WPA3_PSK:     return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:return "WPA2/3";
        default:                     return "?";
    }
}

// ---- A list of Wi-Fi puns for the "PUN AP" broadcaster ----------------------
const char* PUNS[] = {
    "Pretty Fly for a Wi-Fi",
    "The LAN Before Time",
    "Bill Wi the Science Fi",
    "Drop It Like Its Hotspot",
    "Wi-Fight the Inevitable",
    "Abraham Linksys",
    "House LANister",
    "Winternet Is Coming",
    "The Promised LAN",
    "Get Off My LAN",
    "LAN Solo",
    "Nacho Wi-Fi",
    "Silence of the LANs",
    "It Burns When IP",
    "Life in the Fast LAN",
    "Router? I Hardly Know Her",
};
constexpr int N_PUNS = sizeof(PUNS) / sizeof(PUNS[0]);

// ---- Captive sign-in portal state (file-scope so the handlers can see it) ---
WebServer* g_srv = nullptr;
DNSServer* g_dns = nullptr;
int        g_guestCount = 0;
String     g_lastGuest;

String sanitizeName(const String& in) {
    String o;
    for (size_t i = 0; i < in.length() && o.length() < 48; ++i) {
        char c = in[i];
        if (c == ',' || c == '\n' || c == '\r') c = ' ';
        if (c >= 32 && c < 127) o += c;
    }
    o.trim();
    return o;
}

const char* PAGE_FORM =
    "<!doctype html><html><head><meta name=viewport "
    "content='width=device-width,initial-scale=1'><title>Guest Wi-Fi</title>"
    "<style>body{font-family:sans-serif;background:#111;color:#eee;margin:0;"
    "padding:28px;text-align:center}h1{font-size:22px}input{font-size:18px;"
    "padding:12px;width:80%;margin:14px 0;border-radius:8px;border:none}"
    "button{font-size:18px;padding:12px 28px;border:none;border-radius:8px;"
    "background:#D7171E;color:#fff}.c{max-width:420px;margin:0 auto}"
    "p{color:#aaa}</style></head><body><div class=c>"
    "<h1>Welcome &ndash; please sign in</h1>"
    "<p>Enter your name to join the guest network.</p>"
    "<form method=POST action=/signin>"
    "<input name=name placeholder='Your name' autofocus required maxlength=48>"
    "<br><button type=submit>Sign in</button></form>"
    "<p>Operated by the network owner. Your name is recorded on sign-in.</p>"
    "</div></body></html>";

const char* PAGE_THANKS =
    "<!doctype html><html><head><meta name=viewport "
    "content='width=device-width,initial-scale=1'><title>Signed in</title>"
    "<style>body{font-family:sans-serif;background:#111;color:#eee;margin:0;"
    "padding:40px;text-align:center}h1{color:#4caf50}</style></head><body>"
    "<h1>You're signed in</h1><p>Thanks &mdash; enjoy the guest Wi-Fi.</p>"
    "</body></html>";

void handleRoot()  { g_srv->send(200, "text/html", PAGE_FORM); }

void handleSignin() {
    String name = sanitizeName(g_srv->arg("name"));
    if (name.length()) {
        std::string line = std::string(name.c_str()) + "," +
                           std::to_string((unsigned long)millis()) + "\n";
        core::Storage::append(std::string(cfg::DIR_WIFI) + "/guests.csv", line);
        g_lastGuest = name;
        g_guestCount++;
    }
    g_srv->send(200, "text/html", PAGE_THANKS);
}

// Captive-portal catch-all: send every other request back to the sign-in page.
void handleNotFound() {
    g_srv->sendHeader("Location", "http://192.168.4.1/", true);
    g_srv->send(302, "text/plain", "");
}
}  // namespace

namespace apps {

void WifiScan::onEnter() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
    _view = View::Menu;
    if (!_menuBuilt) {
        _menu.add(core::Icon::Scan,     "SCAN",     "Nearby APs, strongest first");
        _menu.add(core::Icon::Analyze,  "CHANNELS", "AP count per 2.4 GHz channel");
        _menu.add(core::Icon::Info,     "DETAIL",   "Full info for one AP");
        _menu.add(core::Icon::Portal,   "PORTAL",   "Your AP + guest sign-in sheet");
        _menu.add(core::Icon::Ap,       "PUN AP",   "Broadcast one pun-named network");
        _menuBuilt = true;
    }
    _menu.draw("WiFi");
}

void WifiScan::onExit() {
    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
}

void WifiScan::scan() {
    UI::clear(); UI::header("WiFi"); UI::banner("Scanning...", UI::cAccent());
    _count = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
    if (_count < 0) _count = 0;
    if (_count > 64) _count = 64;
    for (int i = 0; i < _count; ++i) _order[i] = i;
    for (int i = 0; i < _count; ++i)
        for (int j = i + 1; j < _count; ++j)
            if (WiFi.RSSI(_order[j]) > WiFi.RSSI(_order[i])) {
                int t = _order[i]; _order[i] = _order[j]; _order[j] = t;
            }
    _sel = _top = 0;
}

void WifiScan::drawList() {
    UI::clear();
    char hdr[20]; snprintf(hdr, sizeof(hdr), "%d APs", _count);
    UI::header("WiFi scan", hdr);
    if (_count == 0) { UI::banner("No networks found", UI::cDim()); UI::footer("[ENTER] rescan  [`] back"); return; }
    std::vector<std::string> rows;
    for (int i = 0; i < _count; ++i) {
        int n = _order[i];
        char line[64];
        String ssid = WiFi.SSID(n);
        if (ssid.isEmpty()) ssid = "<hidden>";
        snprintf(line, sizeof(line), "%.16s %ddBm c%d %s",
                 ssid.c_str(), WiFi.RSSI(n), WiFi.channel(n), encStr(WiFi.encryptionType(n)));
        rows.push_back(line);
    }
    UI::list(rows, _sel, _top);
    UI::footer("[ENTER] detail  [r] rescan  [`] back");
}

void WifiScan::drawChannels() {
    UI::clear();
    UI::header("WiFi channels", "2.4 GHz");
    auto& d = M5Cardputer.Display;
    int perCh[15] = {0};
    int maxc = 1;
    for (int i = 0; i < _count; ++i) {
        int ch = WiFi.channel(_order[i]);
        if (ch >= 1 && ch <= 14) { perCh[ch]++; if (perCh[ch] > maxc) maxc = perCh[ch]; }
    }
    const int x0 = 8, top = 24, bottomY = UI::H - 22, usableH = bottomY - top;
    const int colW = (UI::W - 2 * x0) / 14;
    d.setFont(&fonts::Font0);
    d.setTextDatum(top_center);
    for (int ch = 1; ch <= 14; ++ch) {
        int h = (perCh[ch] * usableH) / maxc;
        int x = x0 + (ch - 1) * colW;
        d.fillRect(x + 1, bottomY - h, colW - 2, h, perCh[ch] ? UI::cAccent() : UI::cLine());
        d.setTextColor(UI::cDim(), UI::cBg());
        d.drawString(String(ch).c_str(), x + colW / 2, bottomY + 2);
        if (perCh[ch]) {
            d.setTextColor(UI::cText(), UI::cBg());
            d.drawString(String(perCh[ch]).c_str(), x + colW / 2, bottomY - h - 9);
        }
    }
    UI::footer("busiest channels have the tallest bars   [`] back");
}

void WifiScan::drawDetail() {
    UI::clear();
    UI::header("AP detail");
    if (_count == 0) { UI::banner("Scan first", UI::cDim()); UI::footer("[`] back"); return; }
    int n = _order[_sel];
    auto& d = M5Cardputer.Display;
    d.setTextDatum(top_left);
    int y = 24;
    String ssid = WiFi.SSID(n); if (ssid.isEmpty()) ssid = "<hidden>";
    d.setFont(&fonts::Font2); d.setTextColor(UI::cAccent(), UI::cBg());
    d.drawString(ssid.c_str(), 8, y); y += 20;
    d.setFont(&fonts::Font0); d.setTextColor(UI::cText(), UI::cBg());
    char l[64];
    snprintf(l, sizeof(l), "BSSID %s", WiFi.BSSIDstr(n).c_str()); d.drawString(l, 8, y); y += 13;
    snprintf(l, sizeof(l), "Channel %d", WiFi.channel(n));        d.drawString(l, 8, y); y += 13;
    snprintf(l, sizeof(l), "Signal  %d dBm", WiFi.RSSI(n));       d.drawString(l, 8, y); y += 13;
    snprintf(l, sizeof(l), "Security %s", encStr(WiFi.encryptionType(n))); d.drawString(l, 8, y); y += 13;
    UI::footer("[;/.] other AP   [`] back");
}

// ---------------------------------------------------------------------------
// Simple on-device text editor (used to name the SoftAP). Returns true if the
// user pressed Enter, false on ESC. Del = backspace, printable keys append.
// ---------------------------------------------------------------------------
bool WifiScan::editText(const char* title, std::string& io, size_t maxLen) {
    auto& d = M5Cardputer.Display;
    auto redraw = [&]() {
        UI::clear();
        UI::header(title);
        d.fillRect(10, 46, UI::W - 20, 26, UI::cTile());
        d.drawRect(10, 46, UI::W - 20, 26, UI::cLine());
        d.setFont(&fonts::Font2);
        d.setTextDatum(middle_left);
        d.setTextColor(UI::cText(), UI::cTile());
        std::string shown = io + "_";
        d.drawString(shown.c_str(), 16, 59);
        UI::footer("type a name  [del] erase  [ENTER] ok  [`] cancel");
    };
    redraw();
    for (;;) {
        KeyEvent ev = Input::poll();
        if (ev.key == Key::None) { delay(5); continue; }
        if (ev.key == Key::Ok) { if (!io.empty()) return true; }
        else if (isEscape(ev)) return false;
        else if (isBackspace(ev)) { if (!io.empty()) io.pop_back(); redraw(); }
        else if (ev.key == Key::Char && ev.ch >= 32 && ev.ch < 127) {
            if (io.size() < maxLen) { io.push_back(ev.ch); redraw(); }
        }
    }
}

// ---------------------------------------------------------------------------
// PORTAL: stand up YOUR OWN SoftAP and a guest sign-in page. Visitors who join
// are shown a form; the names they enter are appended to the CSV on SD. You are
// the operator of this network - it does not touch anyone else's AP.
// ---------------------------------------------------------------------------
void WifiScan::runPortal() {
    std::string ssid = _ssid;
    if (!editText("Name your guest AP", ssid, 32)) { _menu.draw("WiFi"); return; }
    _ssid = ssid;

    UI::clear(); UI::header("Guest portal"); UI::banner("Starting AP...", UI::cAccent());
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str());                 // open network you operate
    delay(150);
    IPAddress ip = WiFi.softAPIP();

    g_dns = new DNSServer();
    g_dns->start(53, "*", ip);                 // send every lookup to us (captive)
    g_srv = new WebServer(80);
    g_srv->on("/", handleRoot);
    g_srv->on("/signin", HTTP_POST, handleSignin);
    g_srv->onNotFound(handleNotFound);
    g_srv->begin();
    g_guestCount = 0; g_lastGuest = "";

    auto& d = M5Cardputer.Display;
    int lastCount = -1; uint32_t lastClients = 0, lastTick = 0;
    auto repaint = [&]() {
        UI::clear();
        UI::header("Guest portal", Storage::isReady() ? "CSV on SD" : "no SD");
        d.setFont(&fonts::Font2); d.setTextDatum(top_left);
        int y = 24;
        d.setTextColor(UI::cAccent(), UI::cBg());
        d.drawString(("SSID: " + ssid).substr(0, 30).c_str(), 8, y); y += 20;
        d.setFont(&fonts::Font0); d.setTextColor(UI::cText(), UI::cBg());
        char l[48];
        snprintf(l, sizeof(l), "Join, then open  http://%s", ip.toString().c_str());
        d.drawString(l, 8, y); y += 13;
        snprintf(l, sizeof(l), "Connected now: %u", (unsigned)WiFi.softAPgetStationNum());
        d.drawString(l, 8, y); y += 13;
        snprintf(l, sizeof(l), "Signed in: %d", g_guestCount);
        d.setTextColor(UI::cAccent(), UI::cBg()); d.drawString(l, 8, y); y += 13;
        if (g_lastGuest.length()) {
            d.setTextColor(UI::cText(), UI::cBg());
            snprintf(l, sizeof(l), "last: %.28s", g_lastGuest.c_str());
            d.drawString(l, 8, y);
        }
        UI::footer("[`] stop   names saved to /czero/wifi/guests.csv");
    };
    repaint();

    for (;;) {
        g_dns->processNextRequest();
        g_srv->handleClient();
        KeyEvent ev = Input::poll();
        if (ev.key == Key::Back) break;        // esc OR del both stop the portal
        uint32_t now = millis();
        if (g_guestCount != lastCount || WiFi.softAPgetStationNum() != lastClients ||
            now - lastTick > 1000) {
            lastCount = g_guestCount; lastClients = WiFi.softAPgetStationNum(); lastTick = now;
            repaint();
        }
        delay(2);
    }

    g_srv->stop(); delete g_srv; g_srv = nullptr;
    g_dns->stop(); delete g_dns; g_dns = nullptr;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    _menu.draw("WiFi");
}

// ---------------------------------------------------------------------------
// PUN AP: broadcast exactly ONE SoftAP whose name is a Wi-Fi pun. [ENTER] picks
// a new random pun; ESC stops. It is a single, open, harmless novelty network.
// ---------------------------------------------------------------------------
void WifiScan::runPunAp() {
    int idx = (int)(millis() % N_PUNS);
    auto start = [&](int i) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(PUNS[i]);
        delay(120);
    };
    auto repaint = [&](int i) {
        auto& d = M5Cardputer.Display;
        UI::clear();
        UI::header("Pun AP", "1 network");
        d.setFont(&fonts::Font2); d.setTextDatum(middle_center);
        d.setTextColor(UI::cAccent(), UI::cBg());
        // wrap the pun name across up to two centred lines
        std::string s = PUNS[i];
        if (s.size() <= 20) {
            d.drawString(s.c_str(), UI::W / 2, 54);
        } else {
            size_t sp = s.rfind(' ', 20); if (sp == std::string::npos) sp = 20;
            d.drawString(s.substr(0, sp).c_str(), UI::W / 2, 46);
            d.drawString(s.substr(sp + 1).c_str(), UI::W / 2, 66);
        }
        d.setFont(&fonts::Font0); d.setTextColor(UI::cDim(), UI::cBg());
        d.drawString("broadcasting (open)", UI::W / 2, 86);
        UI::footer("[ENTER] new pun   [`] stop");
    };

    start(idx);
    repaint(idx);
    for (;;) {
        KeyEvent ev = Input::poll();
        if (isEscape(ev)) break;
        if (ev.key == Key::Ok) { idx = (idx + 1 + (int)(millis() % (N_PUNS - 1))) % N_PUNS; start(idx); repaint(idx); }
        delay(5);
    }
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    _menu.draw("WiFi");
}

core::Signal WifiScan::onTick(const core::KeyEvent& ev) {
    switch (_view) {
    case View::Menu:
        if (ev.key == Key::Left)  { _menu.move(-1, 0); _menu.draw("WiFi"); }
        if (ev.key == Key::Right) { _menu.move(+1, 0); _menu.draw("WiFi"); }
        if (ev.key == Key::Up)    { _menu.move(0, -1); _menu.draw("WiFi"); }
        if (ev.key == Key::Down)  { _menu.move(0, +1); _menu.draw("WiFi"); }
        if (ev.key == Key::Back) return Signal::Back;
        if (ev.key == Key::Ok) {
            switch (_menu.selection()) {
            case 0: scan(); _view = View::List;     drawList();     break;
            case 1: scan(); _view = View::Channels; drawChannels(); break;
            case 2: scan(); _view = View::Detail;   drawDetail();   break;
            case 3: runPortal(); break;      // returns to Menu on its own
            case 4: runPunAp();  break;      // returns to Menu on its own
            }
        }
        break;

    case View::List:
        if (ev.key == Key::Back) { _view = View::Menu; _menu.draw("WiFi"); break; }
        if (ev.key == Key::Char && (ev.ch == 'r' || ev.ch == 'R')) { scan(); drawList(); }
        if (_count > 0) {
            if (ev.key == Key::Up)   { UI::move(-1, _count, _sel, _top); drawList(); }
            if (ev.key == Key::Down) { UI::move(+1, _count, _sel, _top); drawList(); }
            if (ev.key == Key::Ok)   { _view = View::Detail; drawDetail(); }
        }
        break;

    case View::Channels:
        if (ev.key == Key::Back) { _view = View::Menu; _menu.draw("WiFi"); break; }
        if (ev.key == Key::Ok)   { scan(); drawChannels(); }
        break;

    case View::Detail:
        if (ev.key == Key::Back) { _view = View::List; drawList(); break; }
        if (_count > 0) {
            if (ev.key == Key::Up)   { UI::move(-1, _count, _sel, _top); drawDetail(); }
            if (ev.key == Key::Down) { UI::move(+1, _count, _sel, _top); drawDetail(); }
        }
        break;
    }
    return Signal::Stay;
}

}  // namespace apps
