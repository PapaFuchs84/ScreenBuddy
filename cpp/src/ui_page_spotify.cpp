// © 2026 FuchsLabs
// Datenquelle Spotify (Client-ID, Anmeldung per Browser).
#include "ui_pages.h"

#include <windows.h>

#include "app.h"
#include "spotify.h"
#include "ui_logic.h"
#include "util.h"

namespace fl::ui {
namespace {

void copy_to_clipboard(HWND owner, const std::string& text) {
    std::wstring w = widen(text);
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (w.size() + 1) * sizeof(wchar_t));
    if (mem) {
        if (void* p = GlobalLock(mem)) {
            memcpy(p, w.c_str(), (w.size() + 1) * sizeof(wchar_t));
            GlobalUnlock(mem);
            if (!SetClipboardData(CF_UNICODETEXT, mem)) GlobalFree(mem);
        } else {
            GlobalFree(mem);
        }
    }
    CloseClipboard();
}

class SpotifyPage : public Page {
public:
    PageInfo info() const override {
        return {"spotify", Group::Sources, Text(L"Spotify"),
                T(L"Aktuellen Titel mit Cover anzeigen und per Touch steuern", L"Show the current track with cover art and control it by touch")};
    }
    bool active(const ojson& cfg) const override { return flag(cfg, "spotify", "enabled"); }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        Card* page = root.add<Card>(T(L"Seite", L"Page"));
        form_.toggle(*page, "spotify", "enabled", T(L"Seite anzeigen", L"Show page"));
        form_.text(*page, "spotify", "title", T(L"Titel", L"Title"));

        Card* setup = root.add<Card>(T(L"Spotify-App anlegen", L"Create a Spotify app"),
            T(L"Einmalig, kostenlos und nur für dich. 1) Auf developer.spotify.com › Dashboard eine App anlegen. "
              L"2) Als Redirect URI genau die Adresse unten eintragen. 3) Die Client-ID hier einfügen und speichern. "
              L"4) Auf „Bei Spotify anmelden“ klicken.",
              L"One time, free and just for you. 1) Create an app at developer.spotify.com › Dashboard. "
              L"2) Add exactly the address below as Redirect URI. 3) Paste the client ID here and save. "
              L"4) Click “Sign in to Spotify”."));
        redirect_ = setup->row<Label>(T(L"Redirect URI", L"Redirect URI"), Text(),
                                      Text(widen(spotify::redirect_uri(host.app().cfg()["ui"].value("port", 8765)))), Font::Small, Label::Normal);
        auto copy = setup->add<ButtonRow>();
        copy->add(T(L"Redirect URI kopieren", L"Copy redirect URI"), Button::Secondary, [this] {
            copy_to_clipboard(nullptr, spotify::redirect_uri(host_->app().cfg()["ui"].value("port", 8765)));
            host_->toast(T(L"Kopiert.", L"Copied."), ToastKind::Success);
        });
        form_.text(*setup, "spotify", "client_id", T(L"Client-ID", L"Client ID"),
                   T(L"32 Zeichen aus dem Spotify-Dashboard. Es wird kein Client Secret gebraucht.",
                     L"32 characters from the Spotify dashboard. No client secret is needed."),
                   Text(L"0123456789abcdef0123456789abcdef"));

        Card* conn = root.add<Card>(T(L"Verbindung", L"Connection"),
                                    T(L"Steuern per Tipp braucht Touch und Spotify Premium. Die Anzeige geht auch ohne Premium.",
                                      L"Controlling by tap needs touch and Spotify Premium. Showing the track works without Premium."));
        state_ = conn->row<Label>(T(L"Status", L"Status"), Text(), Text(L"…"), Font::Small, Label::Muted);
        auto buttons = conn->add<ButtonRow>();
        login_ = buttons->add(T(L"Bei Spotify anmelden", L"Sign in to Spotify"), Button::Primary, [this] { login(); });
        buttons->add(T(L"Abmelden", L"Sign out"), Button::Secondary, [this] { logout(); });
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override { form_.collect(patch); }
    void tick(Host&, const Status& s) override {
        if (!state_ || !s.valid) return;
        Label::Tone tone = Label::Muted;
        state_->text = connection_text(s.connections.contains("spotify") ? s.connections.at("spotify") : ojson(), tone);
        state_->tone = tone;
    }

private:
    Form form_;
    Host* host_ = nullptr;
    Label *state_ = nullptr, *redirect_ = nullptr;
    Button* login_ = nullptr;

    void login() {
        auto* client = host_->app().spotify();
        if (!client) {
            host_->toast(T(L"Zuerst „Seite anzeigen“ einschalten, eine gültige Client-ID eintragen und speichern.",
                           L"First turn on “Show page”, enter a valid client ID and save."), ToastKind::Error);
            return;
        }
        try {
            client->begin_login();
            host_->toast(T(L"Der Browser öffnet sich – dort bei Spotify zustimmen.", L"Your browser opens – approve access there."), ToastKind::Info);
        } catch (const spotify::SpotifyError& e) {
            host_->toast(Text(widen(e.what())), ToastKind::Error);
        }
    }
    void logout() {
        if (auto* client = host_->app().spotify()) client->logout();
        host_->toast(T(L"Abgemeldet.", L"Signed out."), ToastKind::Info);
    }
};

}  // namespace
std::unique_ptr<Page> make_spotify_page() { return std::make_unique<SpotifyPage>(); }
}  // namespace fl::ui