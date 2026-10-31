// © 2026 FuchsLabs
#include "key_help.h"

#include <algorithm>
#include <cctype>

namespace fl::keyhelp {
namespace {

const char* const kHosts[] = {
    "discord.com", "steamcommunity.com", "xbl.io", "www.playstation.com", "ca.account.sony.com", "www.themoviedb.org",
    "developer.spotify.com", "my.home-assistant.io", "www.home-assistant.io",
};

std::vector<Help> build() {
    std::vector<Help> v;
    v.push_back({"discord", L"So bekommst du den Bot-Token", L"How to get the bot token",
        {L"Entwicklerseite öffnen und mit Discord anmelden; oben rechts „New Application“, Namen vergeben.",
         L"Links auf „Bot“ klicken, dann „Reset Token“ und „Copy“. Der Token wird nur einmal angezeigt.",
         L"Auf der Bot-Seite „Server Members Intent“ und „Presence Intent“ einschalten.",
         L"Links „OAuth2“ › URL Generator: Scope „bot“ wählen, den Link öffnen und den Bot zu deinem Server einladen.",
         L"Token hier einfügen und speichern. Er wird verschlüsselt abgelegt; nie weitergeben."},
        {L"Open the developer page and sign in with Discord; top right “New Application”, give it a name.",
         L"Click “Bot” on the left, then “Reset Token” and “Copy”. The token is shown only once.",
         L"On the Bot page switch on “Server Members Intent” and “Presence Intent”.",
         L"On the left “OAuth2” › URL Generator: pick scope “bot”, open the link and invite the bot to your server.",
         L"Paste the token here and save. It is stored encrypted; never share it."},
        {{"https://discord.com/developers/applications", L"Discord-Entwicklerseite öffnen", L"Open Discord developer page"}}});
    v.push_back({"steam", L"So bekommst du den Steam-Schlüssel", L"How to get the Steam key",
        {L"Seite öffnen und bei Steam anmelden.",
         L"Bei „Domain Name“ localhost eintragen, Haken setzen, „Register“ klicken.",
         L"Den langen Schlüssel kopieren und hier einfügen.",
         L"Wichtig: Im Steam-Profil unter Datenschutz muss die Freundesliste „Öffentlich“ sein."},
        {L"Open the page and sign in to Steam.",
         L"Enter localhost as “Domain Name”, tick the box and click “Register”.",
         L"Copy the long key and paste it here.",
         L"Important: in your Steam profile privacy settings the friends list must be “Public”."},
        {{"https://steamcommunity.com/dev/apikey", L"Steam-Schlüssel holen", L"Get Steam key"},
         {"https://steamcommunity.com/my/edit/settings", L"Datenschutz öffnen", L"Open privacy settings"}}});
    v.push_back({"xbox", L"So bekommst du den Xbox-Schlüssel (OpenXBL)", L"How to get the Xbox key (OpenXBL)",
        {L"OpenXBL öffnen und mit deinem Microsoft-/Xbox-Konto anmelden.",
         L"Im Bereich „Profile“/Dashboard einen API-Schlüssel erzeugen oder den vorhandenen kopieren.",
         L"Schlüssel hier einfügen, speichern und „Verbindung prüfen“ klicken."},
        {L"Open OpenXBL and sign in with your Microsoft/Xbox account.",
         L"In the “Profile”/dashboard area create an API key or copy the existing one.",
         L"Paste the key here, save and click “Check connection”."},
        {{"https://xbl.io/", L"OpenXBL öffnen", L"Open OpenXBL"}}});
    v.push_back({"psn", L"So bekommst du den PlayStation-Code (NPSSO)", L"How to get the PlayStation code (NPSSO)",
        {L"Auf playstation.com im Browser mit deinem PlayStation-Konto anmelden.",
         L"Im selben Browser die zweite Seite öffnen. Dort steht eine kurze Zeile mit „npsso“ und einem langen Code.",
         L"Nur den Code kopieren (oder die ganze Zeile) und hier einfügen, speichern, „Verbindung prüfen“ klicken.",
         L"Wichtig: Dieser Code wirkt wie dein Passwort – nie weitergeben. Er läuft nach etwa zwei Monaten ab, dann einfach neu holen.",
         L"Hinweis: Sony bietet dafür keine offizielle Schnittstelle. Die Nutzung ist inoffiziell und geschieht auf eigenes Risiko."},
        {L"Sign in to playstation.com in your browser with your PlayStation account.",
         L"In the same browser open the second page. It shows a short line with “npsso” and a long code.",
         L"Copy only the code (or the whole line), paste it here, save and click “Check connection”.",
         L"Important: this code works like your password – never share it. It expires after about two months; just fetch a new one.",
         L"Note: Sony offers no official interface for this. Use is unofficial and at your own risk."},
        {{"https://www.playstation.com/", L"PlayStation-Seite öffnen", L"Open PlayStation website"},
         {"https://ca.account.sony.com/api/v1/ssocookie", L"Code-Seite öffnen (nach der Anmeldung)", L"Open code page (after signing in)"}}});
    v.push_back({"tmdb", L"So bekommst du den TMDB-Key", L"How to get the TMDB key",
        {L"Kostenloses Konto bei themoviedb.org anlegen oder anmelden.",
         L"Einstellungen › API: einen API-Key beantragen (Privatnutzung, Angaben nach Wahrheit).",
         L"„API Key“ (v3) oder „API Read Access Token“ kopieren und hier einfügen."},
        {L"Create or sign in to a free account at themoviedb.org.",
         L"Settings › API: request an API key (personal use, answer truthfully).",
         L"Copy the “API Key” (v3) or “API Read Access Token” and paste it here."},
        {{"https://www.themoviedb.org/settings/api", L"TMDB-API-Seite öffnen", L"Open TMDB API page"}}});
    v.push_back({"spotify", L"So bekommst du die Spotify-Client-ID", L"How to get the Spotify client ID",
        {L"Dashboard öffnen, anmelden und „Create app“ klicken.",
         L"Als Redirect URI genau die Adresse aus dem Feld unten eintragen (Kopieren-Knopf nutzen).",
         L"Nach dem Speichern bei der App „Settings“ öffnen und die Client-ID kopieren. Ein Client Secret ist nicht nötig.",
         L"Client-ID hier einfügen, speichern, dann „Bei Spotify anmelden“."},
        {L"Open the dashboard, sign in and click “Create app”.",
         L"Enter exactly the address from the field below as Redirect URI (use the copy button).",
         L"After saving open the app’s “Settings” and copy the client ID. No client secret is needed.",
         L"Paste the client ID here, save, then “Sign in to Spotify”."},
        {{"https://developer.spotify.com/dashboard", L"Spotify-Dashboard öffnen", L"Open Spotify dashboard"}}});
    v.push_back({"homeassistant", L"So bekommst du das Zugriffstoken", L"How to get the access token",
        {L"In Home Assistant unten links auf deinen Namen klicken (Profil) und den Reiter „Sicherheit“ öffnen.",
         L"Ganz unten bei „Langlebige Zugriffstoken“ auf „Token erstellen“ klicken.",
         L"Namen vergeben (z. B. „Display“), bestätigen und das Token sofort kopieren – es wird nur einmal angezeigt.",
         L"Hier einfügen und speichern. Es wird verschlüsselt abgelegt."},
        {L"In Home Assistant click your name at the bottom left (profile) and open the “Security” tab.",
         L"At the very bottom under “Long-lived access tokens” click “Create token”.",
         L"Enter a name (e.g. “Display”), confirm and copy the token right away – it is shown only once.",
         L"Paste it here and save. It is stored encrypted."},
        {{"https://my.home-assistant.io/redirect/profile/", L"Profil in Home Assistant öffnen", L"Open profile in Home Assistant"},
         {"https://www.home-assistant.io/docs/authentication/#your-account-profile", L"Anleitung von Home Assistant", L"Home Assistant guide"}}});
    return v;
}

}  // namespace

const std::vector<Help>& all() {
    static const std::vector<Help> v = build();
    return v;
}

const Help* find(const std::string& id) {
    for (const auto& h : all()) if (h.id == id) return &h;
    return nullptr;
}

bool is_official_url(const std::string& url) {
    static const std::string scheme = "https://";
    if (url.compare(0, scheme.size(), scheme) != 0) return false;
    std::string rest = url.substr(scheme.size());
    const size_t end = rest.find_first_of("/?#");
    std::string host = rest.substr(0, end);
    if (host.empty() || host.find_first_of("@:\\ ") != std::string::npos) return false;
    for (unsigned char c : url) if (c <= 0x20 || c >= 0x7f) return false;
    std::transform(host.begin(), host.end(), host.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return std::any_of(std::begin(kHosts), std::end(kHosts), [&](const char* h) { return host == h; });
}

}  // namespace fl::keyhelp
