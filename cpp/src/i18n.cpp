// © 2026 FuchsLabs
#include "i18n.h"

#include <regex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "util.h"

namespace fl::i18n {

static std::string g_lang = "de";

void set_lang(const std::string& l) { g_lang = (l == "en") ? "en" : "de"; }
const std::string& lang() { return g_lang; }

static const std::unordered_map<std::string, std::string>& en() {
    static const std::unordered_map<std::string, std::string> m = {
        // Display: Titel, Kopf- und Fußzeilen, Hinweise
        {"Discord-Freunde", "Discord friends"},
        {"Freunde", "Friends"},
        {"Battlefield 6 · Rangliste", "Battlefield 6 · Leaderboard"},
        {"Blog-Updates", "Blog updates"},
        {"Datenträger", "Drive"},
        {"Netzwerk", "Network"},
        {"Takt", "Clock"},
        {"Lüfter", "Fan"},
        {"belegt", "used"},
        {"ohne HWiNFO", "without HWiNFO"},
        {"Online", "Online"},
        {"Abwesend", "Away"},
        {"Bitte nicht stören", "Do not disturb"},
        {"Offline", "Offline"},
        {"Gerade ist niemand online", "Nobody is online right now"},
        {"Keine Verbindung zu Discord …", "No connection to Discord …"},
        {"Keine Spieler in config.toml", "No players set up"},
        {"Lade Battlefield-Profile …", "Loading Battlefield profiles …"},
        {"Rang unbekannt", "Rank unknown"},
        {"nicht gefunden", "not found"},
        {"Lade Feeds …", "Loading feeds …"},
        {"Keine Beiträge gefunden", "No posts found"},
        {"gerade eben", "just now"},
        {"Willkommen!", "Welcome!"},
        {"kein Netzwerk", "no network"},
        // Meldungen (Display und Einstellungsseite)
        {"Noch keine Konten verbunden – bitte in den Einstellungen einrichten",
         "No accounts connected yet – please set them up in the settings"},
        {"Verbindung zum Display verloren – wird erneut versucht", "Lost connection to the display – retrying"},
        {"Die Hersteller-App des Displays läuft und belegt den Anschluss",
         "The display's vendor app is running and blocking the port"},
        {"Display nicht gefunden – USB-Kabel und Anschluss prüfen", "Display not found – check the USB cable and port"},
        {"Display nicht erreichbar", "Display not reachable"},
        {"Bildschirm nicht gefunden", "Screen not found"},
        {"Kein zweiter Bildschirm angeschlossen", "No second screen connected"},
        {"Anzeigefenster konnte nicht geöffnet werden", "Could not open the display window"},
        {"Anzeigefenster ist geschlossen", "The display window is closed"},
        {"Verbinde mit Discord …", "Connecting to Discord …"},
        {"Kein Bot-Token – bitte in den Einstellungen eintragen", "No bot token – please enter it in the settings"},
        {"Der Bot ist auf keinem Server – bitte einladen (siehe Anleitung)",
         "The bot is not on any server – please invite it (see the guide)"},
        {"Discord verweigert den Zugriff: im Developer Portal beide „Intents“ einschalten",
         "Discord denied access: turn on both “Intents” in the Developer Portal"},
        {"Bot-Token ungültig – bitte in den Einstellungen neu eintragen",
         "Invalid bot token – please enter it again in the settings"},
        {"Keine Verbindung zu Discord", "No connection to Discord"},
        {"Steam: Schlüssel ungültig oder die Freundesliste im Profil ist nicht öffentlich",
         "Steam: invalid key, or the friends list in your profile is not public"},
        {"Steam: zu viele Abrufe, ich versuche es später wieder", "Steam: too many requests, trying again later"},
        {"Steam ist nicht erreichbar", "Steam is not reachable"},
        {"Steam: Profil nicht gefunden – bitte den Link aus der Adresszeile deines Profils einfügen",
         "Steam: profile not found – please paste the link from your profile's address bar"},
        {"Xbox: OpenXBL-Schlüssel ungültig", "Xbox: invalid OpenXBL key"},
        {"Xbox: Abruflimit von OpenXBL erreicht, ich versuche es später wieder",
         "Xbox: OpenXBL rate limit reached, trying again later"},
        {"OpenXBL ist nicht erreichbar", "OpenXBL is not reachable"},
        {"Name fehlt", "Name missing"},
        {"Spieler nicht gefunden", "Player not found"},
        {"Adresse fehlt", "Address missing"},
        {"Kein Feed gefunden", "No feed found"},
        {"Ungültige oder unsichere Adresse", "Invalid or unsafe address"},
        {"Bitte zuerst den Schlüssel eintragen", "Please enter the key first"},
        {"Bitte den Link zu deinem Steam-Profil eintragen", "Please enter the link to your Steam profile"},
        {"Konnte die Hersteller-App nicht beenden", "Could not close the vendor app"},
        {"Die Windows-Abfrage wurde abgelehnt", "The Windows prompt was declined"},
        {"Ja/Nein erwartet", "yes/no expected"},
        {"Liste erwartet", "list expected"},
        {"Zahl erwartet", "number expected"},
        {"Farbe als #RRGGBB erwartet", "colour expected as #RRGGBB"},
        // Beispieldaten (--demo, Vorschaubilder)
        {"gleich zurück", "back soon"},
        {"Tippfehler", "Typo"},
        {"Beispiel-Blog", "Example blog"},
        {"Noch ein Blog", "Another blog"},
        {"Ein kurzer Titel für die Vorschau", "A short title for the preview"},
        {"Zweiter Beitrag mit etwas längerem Titel, der umbricht", "Second post with a slightly longer title that wraps"},
        // Infobereich
        {"Einstellungen öffnen", "Open settings"},
        {"Neu starten", "Restart"},
        {"Beenden", "Quit"},
        // Installer
        {"Update auf Version ", "Update to version "},
        {"Installation · Version ", "Setup · version "},
        {"Update", "Update"},
        {"Installation", "Setup"},
        {"Die App wird für dein Benutzerkonto installiert (keine Administratorrechte nötig):",
         "The app is installed for your user account (no administrator rights needed):"},
        {"Mit Windows starten (nur das Fuchs-Symbol im Infobereich)", "Start with Windows (just the fox icon in the tray)"},
        {"Eintrag im Startmenü", "Start menu entry"},
        {"Alte Version 1.x stilllegen (Einstellungen werden übernommen)", "Retire old version 1.x (settings are kept)"},
        {"Abbrechen", "Cancel"},
        {"Installieren", "Install"},
        {"Aktualisieren", "Update"},
        {"Schließen", "Close"},
        {"Wird installiert …", "Installing …"},
        {"Laufende App wird beendet …", "Closing the running app …"},
        {"Dateien werden kopiert …", "Copying files …"},
        {"Verknüpfungen werden angelegt …", "Creating shortcuts …"},
        {"Fertig! Die App startet jetzt.", "Done! The app is starting now."},
        {"Kopieren fehlgeschlagen", "Copying failed"},
        {"Die laufende App ließ sich nicht beenden. Bitte über das Fuchs-Symbol beenden und erneut versuchen.",
         "Could not close the running app. Please quit it via the fox icon and try again."},
        {"FuchsLabs USB-Display entfernen?", "Remove FuchsLabs USB-Display?"},
        {"Die App wird beendet und vom PC entfernt.", "The app will be closed and removed from this PC."},
        {"Auch Einstellungen und Zugangsdaten löschen", "Also delete settings and access keys"},
    };
    return m;
}

// Muster für zusammengesetzte Texte; Reihenfolge zählt (std::regex auf UTF-8-Bytes: die Muster enthalten
// Umlaute nur als feste Zeichenfolgen, (.+) und (\S+) greifen ganze Bytes, das passt für UTF-8)
static const std::vector<std::pair<std::regex, std::string>>& patterns() {
    static const std::vector<std::pair<std::regex, std::string>> p = [] {
        std::vector<std::pair<const char*, const char*>> raw = {
            {R"(^Spielt (.+)$)", "Playing $1"},
            {R"(^Streamt (.+)$)", "Streaming $1"},
            {R"(^Hört (.+)$)", "Listening to $1"},
            {R"(^Schaut (.+)$)", "Watching $1"},
            {R"(^Tritt an in (.+)$)", "Competing in $1"},
            {R"(^Seite (\d+)/(\d+)(.*)$)", "Page $1/$2$3"},
            {R"(^Stand (\d+:\d+)$)", "Updated $1"},
            {R"(^Rang (\S+)(.*)$)", "Rank $1$2"},
            {R"(^(\d+) % Siege$)", "$1 % wins"},
            {R"(^(\d+) Spieler$)", "$1 players"},
            {R"(^vor (\d+) min$)", "$1 min ago"},
            {R"(^vor (\d+) h$)", "$1 h ago"},
            {R"(^vor (\d+) Tagen$)", "$1 days ago"},
            {R"(^(\d+) Server$)", "$1 servers"},
            {R"(^(\d+) Freunde, (\d+) online$)", "$1 friends, $2 online"},
            {R"(^Steam antwortet mit Fehler (\d+)$)", "Steam replied with error $1"},
            {R"(^OpenXBL antwortet mit Fehler (\d+)$)", "OpenXBL replied with error $1"},
            {R"(^(\w+): unerwarteter Fehler$)", "$1: unexpected error"},
            {R"(^erlaubt: (.+)$)", "allowed: $1"},
            {R"(^(.+) – Bildschirm in den Einstellungen prüfen$)", "$1 – check the screen in the settings"},
            {R"(^Öffne die Einstellungen über das Fuchs-Symbol im Infobereich\. Lokale Adresse: (.+)$)",
             "Open settings via the fox icon in the notification area. Local address: $1"},
        };
        std::vector<std::pair<std::regex, std::string>> out;
        for (auto& [rx, rep] : raw) out.emplace_back(std::regex(rx), rep);
        return out;
    }();
    return p;
}

std::string tr(const std::string& text) {
    if (g_lang == "de" || text.empty()) return text;
    auto& m = en();
    if (auto it = m.find(text); it != m.end()) return it->second;
    if (text.find(" · ") != std::string::npos) {  // z. B. „Spielt X · 42 min“, „Fehler · Fehler“
        auto parts = split(text, " · ");
        std::vector<std::string> out;
        bool changed = false;
        for (auto& p : parts) {
            out.push_back(tr(p));
            changed |= out.back() != p;
        }
        if (changed) return join(out, " · ");
    }
    for (auto& [rx, rep] : patterns())
        if (std::regex_match(text, rx)) return std::regex_replace(text, rx, rep);
    if (auto i = text.find(": "); i != std::string::npos) {  // „display.port: erlaubt: …“
        std::string head = text.substr(0, i), rest = text.substr(i + 2);
        if (head.find('.') != std::string::npos && head.find(' ') == std::string::npos) return head + ": " + tr(rest);
    }
    return text;
}

}  // namespace fl::i18n
